#include "myclient.h"
using namespace std;
#pragma warning(disable : 4996)
#pragma warning(disable : 6011)
void init()
{
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
    server_addr.sin_family = AF_INET; // 使用IPV4
    server_addr.sin_port = htons(4001);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server = socket(AF_INET, SOCK_DGRAM, 0);
    bind(server, (SOCKADDR *)&server_addr, sizeof(server_addr)); // 绑定套接字，进入监听状态
    cout << "等待连接....." << endl;
}
bool interact(SOCKET &sockServ, SOCKADDR_IN &ClientAddr, int &ClientAddrLen, string type, int flag)
{
    // type：hello 或 goodbye
    int sendsign = 0, recvsign = 0;
    if (type == "hello")
    {
        sendsign = 0x4; // 00000100
        recvsign = 0x5; // 00000101 ack+syn
    }
    else
    {
        sendsign = 0x2; // 00000010
        recvsign = 0x3; // 00000011 ack+fin
    }
    Header recvh;
    Header sendh;
    char *Buffer = new char[sizeof(recvh)];
    int res = 0;

    SOCKADDR_IN client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(4002);              // 替换为客户端期望端口
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 允许任何IP地址

    if (flag && bind(sockServ, (SOCKADDR *)&client_addr, sizeof(client_addr)) == SOCKET_ERROR)
    {
        cout << "绑定失败!" << endl;
        return false;
    }
    sendh.setHeader(0, sendsign, 0); // 设置flag
    setSum(sendh);
    memcpy(Buffer, &sendh, sizeof(sendh));
    // 发送第一次握手信息
    res = sendto(sockServ, Buffer, sizeof(recvh), 0, (sockaddr *)&ClientAddr, ClientAddrLen);

    if (res == -1)
    {
        cout << "第一次" << type << "失败 " << endl;
        return false;
    }
    else
        cout << "第一次" << type << " 成功 " << endl;
    long long head;
    QueryPerformanceCounter((LARGE_INTEGER *)&head);
    u_long mode = 1; // 设置非阻塞模式
    ioctlsocket(server, FIONBIO, &mode);
    while (recvfrom(sockServ, Buffer, sizeof(recvh), 0, (sockaddr *)&ClientAddr, &ClientAddrLen) <= 0)
    {
        if (time(head) > TIMEOUT)
        {
            memcpy(Buffer, &sendh, sizeof(sendh));
            sendto(sockServ, Buffer, sizeof(recvh), 0, (sockaddr *)&ClientAddr, ClientAddrLen);
            QueryPerformanceCounter((LARGE_INTEGER *)&head);
            cout << "第一次" << type << "超时，重新发送......" << endl;
            continue;
        }
    }
    memcpy(&recvh, Buffer, sizeof(recvh)); // 解析接受的头部信息
    if (check_sign(recvh, recvsign))
    {

        cout << "第二次" << type << " 成功 " << endl;
        cout << "---------------------------- 成功 ---------------------------------" << endl;
        return true;
    }
    else
    {
        cout << "----------------------------成功---------------------------------" << endl;
        return false;
    }
}

void packet_send(SOCKET &clientsocket, SOCKADDR_IN &serveraddr, int &addrlen, char *message, int len, bool END)
{
    // 设置头部信息
    Header recvh;
    Packet sendp;
    char *buf = new char[sizeof(recvh)];
    sendp.header.seq = SEQ;
    if (!END)
        sendp.header.flag = 0x0;
    else
        sendp.header.flag = 0x7; // 00000111 结束标志
    sendp.header.datasize = len;
    memcpy(sendp.Buffer, &sendp.header, sizeof(sendp.header)); // 确保头部准确填充
    memcpy(sendp.Buffer + sizeof(sendp.header), message, sizeof(sendp.header) + len);
    u_short tempsum = cksum((u_short *)&sendp.Buffer, sizeof(sendp.Buffer));
    sendp.header.sum = tempsum;
    memcpy(sendp.Buffer, &sendp.header, sizeof(sendp.header));
    sendto(clientsocket, sendp.Buffer, sizeof(sendp.header) + len, 0, (sockaddr *)&serveraddr, addrlen); // 发送数据包
    cout << "发送数据包 " << len << " Byte ";
    sendp.header.show_header();
    long long head;
    QueryPerformanceCounter((LARGE_INTEGER *)&head);
    while (true)
    {
        u_long mode = 1;
        ioctlsocket(clientsocket, FIONBIO, &mode); // 非阻塞模式
        while (recvfrom(clientsocket, buf, sizeof(recvh), 0, (sockaddr *)&serveraddr, &addrlen) <= 0)
        {
            if (time(head) > TIMEOUT)
            {
                sendp.header.setHeader(len, 0x0, SEQ);
                memcpy(sendp.Buffer, &sendp.header, sizeof(sendp.header));
                memcpy(sendp.Buffer + sizeof(sendp.header), message, sizeof(sendp.header) + len);
                u_short tempsum = cksum((u_short *)&sendp, sizeof(sendp.Buffer));
                sendp.header.sum = tempsum;
                memcpy(sendp.Buffer, &sendp.header, sizeof(sendp.header));
                sendto(clientsocket, sendp.Buffer, sizeof(sendp.header) + len, 0, (sockaddr *)&serveraddr, addrlen);
                cout << "超时重传:";
                sendp.header.show_header();

                QueryPerformanceCounter((LARGE_INTEGER *)&head);
                continue;
            }
        }
        memcpy(&recvh, buf, sizeof(recvh));
        if (recvh.flag)
        {
            ACK = recvh.ack;
            if (ckack() && ckend(recvh))
            {
                cout << " 服务器已接收 " << endl;
                break;
            }
            else if (ckack && cksend(recvh))
            {
                cout << "发送成功!" << endl;
                break;
            }
            else
            {
                continue;
            }
        }
    }
    u_long mode = 0;
    ioctlsocket(clientsocket, FIONBIO, &mode); // 恢复为阻塞模式
}
void send(SOCKET &clientsocket, SOCKADDR_IN &serveraddr, int &addrlen, char *message, int len)
{
    // 将数据拆分成4096字节的块进行分包传输
    int num = 0;
    if (len % MAXSIZE == 0)
    {
        num = len / MAXSIZE;
    }
    else
    {
        num = len / MAXSIZE + 1;
    }
    cout << "总包数:" << num << endl;
    SEQ = 0;
    for (int i = 0; i < num; i++)
    {
        cout << "send No.[" << i + 1 << "]packet" << endl;
        if (i != num - 1)
        {
            int templen = MAXSIZE;
            packet_send(clientsocket, serveraddr, addrlen, message + i * MAXSIZE, templen, false);
        }
        else
        {
            int templen = len - (num - 1) * MAXSIZE;
            packet_send(clientsocket, serveraddr, addrlen, message + i * MAXSIZE, templen, true); // 最后一个包
        }
        SEQ++;
    }
}

int main()
{
    init();
    int len = sizeof(server_addr);
    if (!interact(server, server_addr, len, "hello", true))
    {
        return 0;
    }
    vector<string> fileNames;
    string path("D:\\client3");

    vector<string> files;
    intptr_t hFile = 0;          // 查找句柄
    struct _finddata_t fileinfo; // 文件信息
    string p1, p2;
    if ((hFile = _findfirst(p1.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
    {
        do
        {
            if (!(fileinfo.attrib & _A_SUBDIR)) // 忽略子目录
                files.push_back(fileinfo.name);

        } while (_findnext(hFile, &fileinfo) == 0);
        _findclose(hFile);
    }
    int k = 1;
    cout << "文件列表： " << endl;
    for (auto f : files)
    {
        cout << k << ". ";
        cout << f << endl;
        k++;
    }
    int x = 0;
    cout << "请输入要发送的文件序号：" << endl;
    cin >> x;

    if (x > 0 && x <= files.size()) // 检查输入是否有效
    {
        string read_path = path + "\\" + files[x - 1];
        cout << "正在读取文件: " << read_path << endl;

        ifstream fin(read_path.c_str(), ifstream::binary);
        if (!fin)
        {
            cerr << "无法打开文件: " << read_path << endl;
            return 0; // 文件打开失败，退出
        }

        // 读取文件内容
        char *buffer = new char[100000000];
        int i = 0;
        u_short temp = fin.get();
        while (fin)
        {
            buffer[i++] = temp;
            temp = fin.get();
        }
        fin.close();

        // 发送文件名和内容
        send(server, server_addr, len, (char *)(files[x - 1].c_str()), files[x - 1].length());
        long long head, tail, freq;
        QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
        QueryPerformanceCounter((LARGE_INTEGER *)&head);
        send(server, server_addr, len, buffer, i);
        QueryPerformanceCounter((LARGE_INTEGER *)&tail);
        cout << "传输时间为: " << (tail - head) * 1.0 / freq << " s" << endl;
        cout << "吞吐率为: " << ((double)i) / ((tail - head) * 1.0 / freq) << " byte/s" << endl;

        string save_path = files[x - 1];
        ofstream fout(save_path.c_str(), ofstream::binary);
        if (!fout)
        {
            cerr << "无法保存文件: " << save_path << endl;
            delete[] buffer;
            return 0;
        }

        fout.write(buffer, i); // 写入文件内容
        fout.close();

        delete[] buffer; // 释放动态分配的内存
    }
    else
    {
        cout << "无效的文件序号!" << endl;
    }

    interact(server, server_addr, len, "goodbye", false);
    system("pause");
}

// if (x)
//{
//     string myfile = files[x - 1];
//     /*string myfile = path + "\\" + files[x - 1];*/
//     cout << "starting " << files[x - 1] << endl;
//     ifstream fin(myfile.c_str(), ifstream::binary); // 二进制方式打开文件
//     char *buffer = new char[100000000];
//     int i = 0;
//     u_short temp = fin.get(); // 读取文件内容
//     while (fin)
//     {
//         buffer[i++] = temp;
//         temp = fin.get();
//     }
//     fin.close();
//     long long head, tail, freq;
//     send(server, server_addr, len, (char *)(myfile.c_str()), myfile.length());
//     QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
//     QueryPerformanceCounter((LARGE_INTEGER *)&head);
//     send(server, server_addr, len, buffer, i);
//     QueryPerformanceCounter((LARGE_INTEGER *)&tail);
//     cout << "传输时间为: " << (tail - head) * 1.0 / freq << " s" << endl;
//     cout << "吞吐率为: " << ((double)i) / ((tail - head) * 1.0 / freq) << " byte/s" << endl;
// }