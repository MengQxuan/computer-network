#ifndef MYSERVER_H // 防止重复包含头文件
#define MYSERVER_H

#include <iostream>
#include <WINSOCK2.H> // Windows下的网络套接字库
#include <time.h>
#include <sstream>
#include <windows.h> // Windows API，提供对系统时间、文件等的操作
#include <fstream>
#include <string>
#include <vector>                  // 向量容器，用于存储动态数组
#include <io.h>                    // 文件系统操作函数
#pragma comment(lib, "ws2_32.lib") // 链接 Windows 套接字库

using namespace std;

const int MAXSIZE = 4096; // 定义数据包最大大小为 4096 字节
SOCKADDR_IN server_addr;  // 服务器地址结构
SOCKET server;            // 服务器端套接字
int SEQ = 0;              // 序列号初始化为 0
int ACK = 0;              // 确认号初始化为 0
double TIMEOUT = 2.0;     // 设置超时时间为 2 秒

// 数据包的头部信息
class Header
{
public:
    u_short datasize = 0; // 数据包大小
    u_short sum = 0;      // 校验和
    u_char flag = 0;      // 标志位（SYN, ACK, FIN 等）
    u_char ack = 0;       // 确认号 (ACK = seq + 1)
    u_short seq = 0;      // 序列号

    // 构造函数，初始化 flag 为 0
    Header() { flag = 0; }

    // 设置头部字段
    void setHeader(u_short d, u_char f, u_short se)
    {
        this->datasize = d;
        this->seq = se;
        this->flag = f;
    }

    // 打印头部信息，用于调试
    void show_header()
    {
        cout << " sum: " << (int)this->sum
             << " seq: " << (int)this->seq
             << " flag: " << (int)this->flag << endl;
    }
};

// 数据包结构体
struct Packet
{
    Header header; // 包含头部信息
    char *Buffer;  // 用于存储数据部分

    // 初始化数据缓冲区
    Packet()
    {
        Buffer = new char[MAXSIZE + sizeof(header)](); // 分配存储空间
    }
};

// 计算校验和的函数，用于差错检测
u_short cksum(u_short *buff, int size)
{
    int count = (size + 1) / 2;           // 计算16位数据块数量
    u_short *buf = new u_short[size + 1]; // 分配缓冲区
    memset(buf, 0, size + 1);             // 初始化缓冲区
    memcpy(buf, buff, size);              // 拷贝数据到缓冲区
    u_short sum = 0;                      // 初始化校验和

    // 计算校验和：将每个16位数据加到总和中，检测溢出
    while (count--)
    {
        sum += *buf++;
        if (sum & 0xFFFF0000)
        { // 每次累加后，如果sum溢出（即高于16位），则将其截断到16位，并增加1
            sum &= 0xFFFF;
            sum++;
        }
    }
    return ~(sum & 0xFFFF); // 返回校验和的反码
}

// 计算时间差，用于超时控制
long long time(long long head)
{
    long long tail, freq;
    QueryPerformanceFrequency((LARGE_INTEGER *)&freq); // 获取系统频率
    QueryPerformanceCounter((LARGE_INTEGER *)&tail);   // 获取当前时间
    return (tail - head) / freq;                       // 计算时间差
}

// 设置头部的校验和
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
