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

char name_buffer[20];
char data_buffer[200000000];
const int BUFFER_SIZE = 4096;
double MAX_WAIT_TIME = 200;

struct HEADER
{
    unsigned short sum = 0;
    unsigned short datasize = 0;
    unsigned char flag = 0;
    unsigned char SEQ = 0;
};

unsigned short check_sum(unsigned short *message, int size)
{
    int count = (size + 1) / sizeof(unsigned short);
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

int Shake_hand(SOCKET &socketServer, SOCKADDR_IN &client_addr, int &len)
{
    HEADER header;
    char *buffer = new char[sizeof(header)];
    auto sendAndCheckHeader = [&](unsigned char flag)
    {
        header.flag = flag;
        header.sum = check_sum((unsigned short *)&header, sizeof(header));
        memcpy(buffer, &header, sizeof(header));
        if (sendto(socketServer, buffer, sizeof(header), 0, (sockaddr *)&client_addr, len) == SOCKET_ERROR)
        {
            delete[] buffer; // 清理分配的内存
            return false;
        }
        return true;
    };
    // 接收第一次握手请求
    while (true)
    {
        if (recvfrom(socketServer, buffer, sizeof(header), 0, (sockaddr *)&client_addr, &len) == SOCKET_ERROR)
        {
            delete[] buffer; // 清理分配的内存
            return SOCKET_ERROR;
        }
        memcpy(&header, buffer, sizeof(header));
        if (header.flag == SYN && check_sum((unsigned short *)&header, sizeof(header)) == 0)
        {
            cout << "收到第一次握手请求" << endl;
            break;
        }
    }
    // 发送第二次握手确认
    if (!sendAndCheckHeader(ACK))
    {
        return SOCKET_ERROR;
    }
    cout << "已发送第二次握手请求" << endl;
    clock_t start = clock();
    // 接收第三次握手确认
    while (recvfrom(socketServer, buffer, sizeof(header), 0, (sockaddr *)&client_addr, &len) <= 0)
    {
        if (clock() - start > MAX_WAIT_TIME)
        {
            if (!sendAndCheckHeader(ACK))
            {
                return SOCKET_ERROR;
            }
            cout << "第二次握手超时，开始重传..." << endl;
            start = clock();
        }
    }
    memcpy(&header, buffer, sizeof(header));
    if (header.flag == ACK_SYN && check_sum((unsigned short *)&header, sizeof(header)) == 0)
    {
        cout << "客户端已连接!" << endl;
    }
    else
    {
        delete[] buffer; // 清理分配的内存
        return SOCKET_ERROR;
    }
    delete[] buffer; // 清理分配的内存
    return 1;
}

int Receive_message(SOCKET &socketServer, SOCKADDR_IN &client_addr, int &client_addr_len, char *message)
{
    HEADER header;
    char *buffer = new char[BUFFER_SIZE + sizeof(header)];
    int seq = 0; // 序列号
    int len = 0; // 已读取长度
    auto sendACK = [&](unsigned char ack_seq)
    {
        header.flag = ACK;
        header.datasize = 0;
        header.SEQ = ack_seq;
        header.sum = check_sum((unsigned short *)&header, sizeof(header));
        memcpy(buffer, &header, sizeof(header));
        if (sendto(socketServer, buffer, sizeof(header), 0, (sockaddr *)&client_addr, client_addr_len) == SOCKET_ERROR)
        {
            return false;
        }
        cout << "已发送ACK! SEQ:" << (int)header.SEQ << endl;
        return true;
    };
    while (true)
    {
        int mess_len = recvfrom(socketServer, buffer, sizeof(header) + BUFFER_SIZE, 0, (sockaddr *)&client_addr, &client_addr_len);
        if (mess_len <= 0)
            continue;
        memcpy(&header, buffer, sizeof(header));
        if (header.flag == 0)
        {
            if (header.SEQ == seq) // 序列号匹配
            {
                memcpy(message + len, buffer + sizeof(header), header.datasize);
                len += header.datasize;
                cout << "已接收数据 " << header.datasize << " bytes! SEQ: " << int(header.SEQ) << endl;
                if (!sendACK(seq))
                {
                    delete[] buffer;
                    return SOCKET_ERROR;
                }
                seq = (seq + 1) % 256; // 增加当前序列号并处理溢出
            }
            else // 序列号不匹配
            {
                cout << "已接收数据 " << header.datasize << " bytes! SEQ: " << int(header.SEQ) << endl;
                unsigned char last_ack = (seq - 1 + 256) % 256; // 确保不会出现负数
                if (!sendACK(last_ack))
                {
                    delete[] buffer;
                    return SOCKET_ERROR;
                }
                cout << "待接收序列号: " << seq << " 序列号无效，已重发ACK! SEQ:" << (int)last_ack << endl;
                continue;
            }
        }
        else if (header.flag == END && check_sum((unsigned short *)&header, sizeof(header)) == 0)
        {
            cout << "文件已成功接收" << endl;
            break;
        }
    }
    // 发送结束确认
    header.flag = END;
    header.sum = check_sum((unsigned short *)&header, sizeof(header));
    memcpy(buffer, &header, sizeof(header));
    if (sendto(socketServer, buffer, sizeof(header), 0, (sockaddr *)&client_addr, client_addr_len) == SOCKET_ERROR)
    {
        delete[] buffer;
        return SOCKET_ERROR;
    }
    delete[] buffer; // 清理分配的内存
    return len;
}

int Wave_hand(SOCKET &socketServer, SOCKADDR_IN &client_addr, int &len)
{
    HEADER header;
    char *buffer = new char[sizeof(header)];
    auto sendHeader = [&]()
    {
        header.sum = check_sum((unsigned short *)&header, sizeof(header));
        memcpy(buffer, &header, sizeof(header));
        return sendto(socketServer, buffer, sizeof(header), 0, (sockaddr *)&client_addr, len);
    };
    // 接收第一次挥手请求
    while (true)
    {
        if (recvfrom(socketServer, buffer, sizeof(header), 0, (sockaddr *)&client_addr, &len) == SOCKET_ERROR)
        {
            delete[] buffer; // 清理分配的内存
            return SOCKET_ERROR;
        }
        memcpy(&header, buffer, sizeof(header));
        if (header.flag == FIN && check_sum((unsigned short *)&header, sizeof(header)) == 0)
        {
            cout << "收到第一次挥手请求" << endl;
            break;
        }
    }
    // 发送第二次挥手确认
    header.flag = ACK;
    if (sendHeader() == SOCKET_ERROR)
    {
        delete[] buffer; // 清理分配的内存
        return SOCKET_ERROR;
    }
    cout << "发送第二次挥手请求" << endl;
    cout << "断开连接！" << endl;

    delete[] buffer; // 清理分配的内存
    return 1;
}

int main()
{
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1205);
    server_addr.sin_addr.s_addr = htonl(2130706433);
    server = socket(AF_INET, SOCK_DGRAM, 0);
    bind(server, (SOCKADDR *)&server_addr, sizeof(server_addr));
    cout << "等待连接......" << endl;
    int len = sizeof(server_addr);
    if (Shake_hand(server, server_addr, len) == SOCKET_ERROR)
    {
        return 0;
    }
    int namelen = Receive_message(server, server_addr, len, name_buffer);
    int datalen = Receive_message(server, server_addr, len, data_buffer);
    string a;
    for (int i = 0; i < namelen; i++)
    {
        a = a + name_buffer[i];
    }
    ofstream fout(a.c_str(), ofstream::binary);
    for (int i = 0; i < datalen; i++)
    {
        fout << data_buffer[i];
    }
    fout.close();
    cout << "the file has been downloaded" << endl;
    Wave_hand(server, server_addr, len);
    system("pause");
}