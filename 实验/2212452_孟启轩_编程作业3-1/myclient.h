#ifndef MYCLIENT_H
#define MYCLIENT_H

#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <sstream>
#include <windows.h>
#include <fstream>
#include <string>
#include <vector>
#include <io.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int MAXSIZE = 4096;
SOCKADDR_IN server_addr;
SOCKET server;
int SEQ = 0;
int ACK = 0;
double TIMEOUT = 2.0;

class Header
{
public:
    u_short datasize = 0;
    u_short sum = 0;
    u_char flag = 0;
    u_char ack = 0;
    u_short seq = 0;

    Header() { flag = 0; }

    // 设置头部的值
    void setHeader(u_short d, u_char f, u_short se)
    {
        this->datasize = d;
        this->seq = se;
        this->flag = f;
    }

    // 显示头部信息
    void show_header()
    {
        cout << " sum: " << (int)this->sum
             << " seq: " << (int)this->seq
             << " flag: " << (int)this->flag << endl;
    }
};

// 数据包结构体定义
struct Packet
{
    Header header; // 数据包的头部
    char *Buffer;  // 数据内容

    // 构造函数，分配缓冲区
    Packet()
    {
        Buffer = new char[MAXSIZE + sizeof(header)]();
    }
};

bool ckack();                                // 检查 ACK 的正确性
bool cksend(Header h);                       // 检查是否是发送标志
bool ckend(Header h);                        // 检查是否是结束标志
u_short cksum(u_short *buff, int size);      // 计算校验和
long long time(long long head);              // 计算时间差
void setSum(Header &header);                 // 设置校验和
bool check_sign(Header header, u_char sign); // 检查标志位和校验和是否正确

// 检查 ACK 是否正确
bool ckack()
{
    return ((ACK == (SEQ % 255) + 1)); // 验证 ACK 是否与 SEQ 对应
}

// 检查是否是发送标志
bool cksend(Header h)
{
    return (h.flag == 0x1);
}

// 检查是否是结束标志
bool ckend(Header h)
{
    return (h.flag == 0x7);
}

// 计算校验和
u_short cksum(u_short *buff, int size)
{
    int count = (size + 1) / 2;           // 按 16 位计算块数
    u_short *buf = new u_short[size + 1]; // 分配缓冲区
    memset(buf, 0, size + 1);             // 初始化缓冲区
    memcpy(buf, buff, size);              // 拷贝数据到缓冲区
    u_short sum = 0;                      // 初始化和为 0

    while (count--)
    { // 遍历所有块
        sum += *buf++;
        if (sum & 0xFFFF0000)
        { // 每次累加后，如果sum溢出（即高于16位），则将其截断到16位，并增加1
            sum &= 0xFFFF;
            sum++;
        }
    }
    return ~(sum & 0xFFFF); // 返回取反后的结果
}

// 计算时间差（返回秒）
long long time(long long head)
{
    long long tail, freq;
    QueryPerformanceFrequency((LARGE_INTEGER *)&freq); // 获取频率
    QueryPerformanceCounter((LARGE_INTEGER *)&tail);   // 获取当前时间
    return (tail - head) / freq;
}

// 设置校验和
void setSum(Header &header)
{
    u_short s = cksum((u_short *)&header, sizeof(header));
    header.sum = s;
}

// 检查标志位和校验和是否正确
bool check_sign(Header header, u_char sign)
{
    if (header.flag == sign && cksum((u_short *)&header, sizeof(header)) == 0)
        return true;
    else
        return false;
}

#endif

#pragma once
