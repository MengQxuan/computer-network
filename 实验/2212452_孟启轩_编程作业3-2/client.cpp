#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

#define SYN 0x1
#define ACK 0x2
#define ACK_SYN 0x3
#define FIN 0x4
#define FIN_ACK 0x5
#define END 0x7

SOCKADDR_IN server_addr;
SOCKET server;

char data_buffer[20000000];
const int BUFFER_SIZE = 4096;
const int MAX_WINDOW = 20;
const double MAX_WAIT_TIME = 200;

struct HEADER
{
    unsigned short sum = 0;
    unsigned short datasize = 0;
    unsigned char flag = 0;
    unsigned char SEQ = 0;
};

unsigned short check_sum(unsigned short *message, int size)
{
    int count = (size + 1) / 2;
    unsigned short *buf = (unsigned short *)malloc(size + 1);
    memset(buf, 0, size + 1);
    memcpy(buf, message, size);
    unsigned long sum = 0;
    while (count--)
    {
        sum += *buf++;
        if (sum & 0xffff0000)
        {
            sum &= 0xffff;
            sum++;
        }
    }
    return ~(sum & 0xffff);
}

int Shake_hand(SOCKET &socketClient, SOCKADDR_IN &server_addr, int &server_addr_len)
{
    HEADER header;
    char *buffer = new char[sizeof(header)];
    unsigned short sum;
    auto sendHeader = [&]()
    {
        header.sum = check_sum((unsigned short *)&header, sizeof(header));
        memcpy(buffer, &header, sizeof(header));
        return sendto(socketClient, buffer, sizeof(header), 0, (sockaddr *)&server_addr, server_addr_len);
    };

    // 第一次握手请求
    header.flag = SYN;
    if (sendHeader() == SOCKET_ERROR)
        return SOCKET_ERROR;
    cout << "发送第一次握手请求" << endl;
    clock_t start = clock();
    unsigned long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);
    // 接收第二次握手响应
    while (recvfrom(socketClient, buffer, sizeof(header), 0, (sockaddr *)&server_addr, &server_addr_len) <= 0)
    {
        if (clock() - start > MAX_WAIT_TIME) // 超时，重新传输第一次握手
        {
            header.flag = SYN;
            if (sendHeader() == SOCKET_ERROR)
                return SOCKET_ERROR;
            cout << "第一次握手超时，开始重传..." << endl;
            start = clock();
        }
    }
    memcpy(&header, buffer, sizeof(header));
    if (header.flag != ACK || check_sum((unsigned short *)&header, sizeof(header)) != 0)
    {
        return SOCKET_ERROR;
    }
    cout << "收到第二次握手请求" << endl;
    // 第三次握手确认
    header.flag = ACK_SYN;
    if (sendHeader() == SOCKET_ERROR)
        return SOCKET_ERROR;
    cout << "发送第三次握手请求" << endl;
    cout << "服务器已连接！" << endl;
    delete[] buffer; // 清理分配的内存
    return 1;
}

void Send_Message(SOCKET &socketClient, SOCKADDR_IN &server_addr, int &server_addr_len, char *message, int len)
{
    int package_num = len / BUFFER_SIZE + (len % BUFFER_SIZE != 0); // 数据包数量
    HEADER header;
    char *buffer = new char[BUFFER_SIZE + sizeof(header)];
    auto sendPacket = [&](int pos, bool isEnd)
    {
        int pack_len = (pos == package_num - 1) ? len - pos * BUFFER_SIZE : BUFFER_SIZE;
        header.SEQ = unsigned char(pos % 256);
        header.datasize = pack_len;
        if (isEnd)
        {
            header.flag = END;
        }
        else
        {
            memcpy(buffer, message + pos * BUFFER_SIZE, pack_len);
        }
        header.sum = check_sum((unsigned short *)&header, sizeof(header) + (isEnd ? 0 : pack_len));
        memcpy(buffer, &header, sizeof(header));
        if (sendto(socketClient, buffer, sizeof(header) + (isEnd ? 0 : pack_len), 0, (sockaddr *)&server_addr, server_addr_len) == SOCKET_ERROR)
        {
            return false;
        }
        cout << "发送信息 " << pack_len << " bytes! SEQ:" << int(header.SEQ) << endl;
        return true;
    };

    // 已确认的最后一个数据包   当前窗口的下一个待发送数据包序列号
    int first_pos = -1, last_pos = 0;
    clock_t start;

    while (first_pos < package_num - 1)
    {
        if (last_pos != package_num && last_pos - first_pos < MAX_WINDOW)
        {
            if (!sendPacket(last_pos, false))
                break;
            start = clock();
            last_pos++;
        }
        unsigned long mode = 1;
        ioctlsocket(socketClient, FIONBIO, &mode);
        if (recvfrom(socketClient, buffer, sizeof(header), 0, (sockaddr *)&server_addr, &server_addr_len) > 0)
        {
            memcpy(&header, buffer, sizeof(header));
            if (check_sum((unsigned short *)&header, sizeof(header)) == 0)
            {
                int temp = header.SEQ - first_pos % 256;
                if (temp > 0 || (header.SEQ < MAX_WINDOW && first_pos % 256 + MAX_WINDOW >= 256))
                {
                    first_pos += temp;
                    cout << "发送已被确认! SEQ:" << int(header.SEQ) << endl;
                    cout << "窗口：" << first_pos << "~" << first_pos + MAX_WINDOW << endl;
                }
                else
                {
                    continue; // 忽略重复ACK
                }
            }
            else
            {
                last_pos = first_pos + 1;
                cout << "ERROR！已丢弃未确认数据包" << endl;
            }
        }
        else if (clock() - start > MAX_WAIT_TIME)
        {
            last_pos = first_pos + 1;
            cout << "确认超时，开始重传...";
        }
        mode = 0;
        ioctlsocket(socketClient, FIONBIO, &mode);
    }
    // 发送结束信息
    while (true)
    {
        if (sendPacket(-1, true))
        {
            unsigned long mode = 1;
            ioctlsocket(socketClient, FIONBIO, &mode);
            if (recvfrom(socketClient, buffer, BUFFER_SIZE, 0, (sockaddr *)&server_addr, &server_addr_len) > 0)
            {
                memcpy(&header, buffer, sizeof(header));
                if (check_sum((unsigned short *)&header, sizeof(header)) == 0 && header.flag == END)
                {
                    cout << "对方已成功接收文件!" << endl;
                    break;
                }
            }
            else if (clock() - start > MAX_WAIT_TIME)
            {
                cout << "发送超时! 开始重传..." << endl;
                start = clock();
            }
        }
    }
    delete[] buffer; // 清理分配的内存
}

int Wave_hand(SOCKET &socketClient, SOCKADDR_IN &server_addr, int &server_addr_len)
{
    HEADER header;
    char *buffer = new char[sizeof(header)];
    auto sendHeader = [&]()
    {
        header.sum = check_sum((unsigned short *)&header, sizeof(header));
        memcpy(buffer, &header, sizeof(header));
        return sendto(socketClient, buffer, sizeof(header), 0, (sockaddr *)&server_addr, server_addr_len);
    };

    // 发送挥手请求
    header.flag = FIN;
    if (sendHeader() == SOCKET_ERROR)
    {
        delete[] buffer; // 清理分配的内存
        return SOCKET_ERROR;
    }
    cout << "发送第一次挥手请求" << endl;

    clock_t start = clock();
    unsigned long mode = 1;
    ioctlsocket(socketClient, FIONBIO, &mode);

    // 接收挥手响应
    while (recvfrom(socketClient, buffer, sizeof(header), 0, (sockaddr *)&server_addr, &server_addr_len) <= 0)
    {
        if (clock() - start > MAX_WAIT_TIME) // 超时，重新传输挥手请求
        {
            header.flag = FIN;
            if (sendHeader() == SOCKET_ERROR)
            {
                delete[] buffer; // 清理分配的内存
                return SOCKET_ERROR;
            }
            start = clock();
            cout << "第一次挥手超时，开始重传..." << endl;
        }
    }
    memcpy(&header, buffer, sizeof(header));
    if (header.flag != ACK || check_sum((unsigned short *)&header, sizeof(header)) != 0)
    {
        delete[] buffer; // 清理分配的内存
        return SOCKET_ERROR;
    }
    cout << "收到第二次挥手请求" << endl;
    cout << "断开连接！" << endl;
    delete[] buffer; // 清理分配的内存
    return 1;
}

int main()
{
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1206);
    server_addr.sin_addr.s_addr = htonl(2130706433);
    server = socket(AF_INET, SOCK_DGRAM, 0);
    int len = sizeof(server_addr);
    // 建立连接
    if (Shake_hand(server, server_addr, len) == SOCKET_ERROR)
    {
        return 0;
    }
    string filename;
    cout << "请输入文件名称" << endl;
    cin >> filename;
    ifstream fin(filename.c_str(), ifstream::binary);
    int index = 0;
    unsigned char temp = fin.get();
    while (fin)
    {
        data_buffer[index++] = temp;
        temp = fin.get();
    }
    fin.close();
    Send_Message(server, server_addr, len, (char *)(filename.c_str()), filename.length());
    clock_t start = clock();
    Send_Message(server, server_addr, len, data_buffer, index);
    clock_t end = clock();
    cout << "传输总时间为:" << (float)(end - start) / CLOCKS_PER_SEC << "s" << endl;
    cout << "吞吐率为:" << ((float)index) / ((float)(end - start) / CLOCKS_PER_SEC) << "byte/s" << endl;
    Wave_hand(server, server_addr, len);
    system("pause");
}