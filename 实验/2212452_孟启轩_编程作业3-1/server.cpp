#include "myserver.h"
using namespace std;
#pragma warning(disable : 4996)
#pragma warning(disable : 6011)

char *fileName = new char[20];

void init()
{
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
    server_addr.sin_family = AF_INET; // 使用IPV4
    server_addr.sin_port = htons(4001);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server = socket(AF_INET, SOCK_DGRAM, 0);
    if (bind(server, (SOCKADDR *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) // 绑定套接字，进入监听状态
        cout << "绑定失败!" << endl;
    else
        cout << "等待客户端连接......" << endl;
}
bool interact(SOCKET &sockServ, SOCKADDR_IN &ClientAddr, int &ClientAddrLen, string type)
{
    // type 是hello或goodbye
    int sendsign = 0, recvsign = 0;
    if (type == "hello")
    {
        sendsign = 0x5; // ack+syn
        recvsign = 0x4;
    }
    else
    {
        sendsign = 0x3;
        recvsign = 0x2;
    }

    Header recvh;
    Header sendh;
    char *Buffer = new char[sizeof(recvh)];
    int res = 0;
    // 接收第一次握手信息
    while (true)
    {
        res = recvfrom(sockServ, Buffer, sizeof(recvh), 0, (sockaddr *)&ClientAddr, &ClientAddrLen);
        if (res == -1)
        {
            cout << "第一次 " << type << " 失败" << endl;
            return false;
        }
        memcpy(&recvh, Buffer, sizeof(recvh));
        if (check_sign(recvh, recvsign))
        {
            SEQ = recvh.seq;
            cout << "第一次 " << type << " 成功" << endl;
            sendh.setHeader(0, sendsign, 0);
            setSum(sendh);
            memcpy(Buffer, &sendh, sizeof(sendh));
            res = sendto(sockServ, Buffer, sizeof(sendh), 0, (sockaddr *)&ClientAddr, ClientAddrLen);
            if (res == -1)
            {
                return false;
            }
            else
            {
                cout << "连接/断开成功!" << endl;
                break;
            }
        }
        else
        {
            cout << "连接/断开失败!" << endl;
            return false;
        }
    }
}

int receive(SOCKET &servsocket, SOCKADDR_IN &clientaddr, int &len, char *message)
{
    Packet recvp;
    Header sendh;
    int offset = 0; // 总文件偏移量
    char *buf = new char[sizeof(sendh)];
    SEQ = 0;
    while (true)
    {
        int recvlen = recvfrom(servsocket, recvp.Buffer, sizeof(recvp.header) + MAXSIZE, 0, (sockaddr *)&clientaddr, &len);
        int drop = rand() % 100;
        // 模拟丢包，有 2/100 的概率丢包
        if (drop <2)
            continue;
        memcpy(&recvp.header, recvp.Buffer, sizeof(recvp.header));
        if (recvp.header.flag == 0x7 && cksum((u_short *)&recvp, sizeof(recvp.Buffer)))
        {
            // 仅有头部
            SEQ = recvp.header.seq;
            memcpy(message + offset, recvp.Buffer + sizeof(recvp.header), recvlen - sizeof(recvp.header));
            offset += recvp.header.datasize;
            cout << "the file has been received." << endl;
            break;
        }
        if (recvp.header.flag == 0x0 && cksum((u_short *)&recvp, sizeof(recvp.Buffer)))
        {
            // 判断包
            if (SEQ != recvp.header.seq)
            {
                ACK = (SEQ % 255) + 1;
                sendh.ack = (u_char)ACK;
                sendh.setHeader(0, 0x1, SEQ);
                setSum(sendh);
                memcpy(buf, &sendh, sizeof(sendh));
                sendto(servsocket, buf, sizeof(sendh), 0, (sockaddr *)&clientaddr, len);
                cout << "不是这个数据包，请重新发送!";
                sendh.show_header();
                continue; // 丢弃
            }
            SEQ = recvp.header.seq;
            memcpy(message + offset, recvp.Buffer + sizeof(recvp.header), recvlen - sizeof(recvp.header));
            offset += recvp.header.datasize;
            // 成功收到后，返回ACK
            ACK = (SEQ % 255) + 1;
            sendh.ack = (u_char)ACK;
            sendh.setHeader(0, 0x1, SEQ);
            setSum(sendh);
            memcpy(buf, &sendh, sizeof(sendh));
            sendto(servsocket, buf, sizeof(sendh), 0, (sockaddr *)&clientaddr, len);
            cout << "检查数据包! send ack:" << (int)sendh.ack;
            sendh.show_header();
            SEQ++;
        }
    }

    ACK = (SEQ % 255) + 1;
    sendh.ack = (u_char)ACK;
    sendh.setHeader(0, 0x7, SEQ);
    setSum(sendh);
    memcpy(buf, &sendh, sizeof(sendh));
    if (sendto(servsocket, buf, sizeof(sendh), 0, (sockaddr *)&clientaddr, len) == -1)
    {
        return -1;
    }
    return offset;
}
int main()
{
    init();
    int len = sizeof(server_addr);
    // 建立连接
    interact(server, server_addr, len, "hello");
    char *myfile = new char[100000000];

    int namelen = receive(server, server_addr, len, fileName);
    int filelen = receive(server, server_addr, len, myfile);
    interact(server, server_addr, len, "goodbye");
    ofstream fout;
    string filen = (string)fileName;
    // 查找 ".txt" 或 ".jpg" 的位置
    size_t txtPos = filen.find(".txt");
    size_t jpgPos = filen.find(".jpg");
    // 如果找到了 ".txt"，截断字符串
    if (txtPos != string::npos)
    {
        filen = filen.substr(0, txtPos + 4);
    }
    // 如果找到了 ".jpg"，截断字符串
    else if (jpgPos != string::npos)
    {
        filen = filen.substr(0, jpgPos + 4);
    }
    fout.open(filen.c_str(), ofstream::binary);
    fout.write(myfile, filelen);
    fout.close();
    cout << "the file has been downloaded." << endl;
    system("pause");
}