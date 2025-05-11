#include <iostream>       // 引入输入输出流库
#include <thread>         // 引入线程库
#include <string>         // 引入字符串库
#include <winsock2.h>     // 引入Winsock库，用于网络编程

#pragma comment(lib, "ws2_32.lib")  // 链接Winsock库

#define MAX_BUFFER_SIZE 1024  // 定义最大缓冲区大小
#define SERVER_IP "127.0.0.1"  // 定义服务器IP地址
#define SERVER_PORT 8888       // 定义服务器端口号

using namespace std;

// 函数：接收消息
void receive_messages(SOCKET client_socket) {
    char buffer[MAX_BUFFER_SIZE];  // 定义接收缓冲区
    int bytes_received;             // 定义接收到的字节数

    while (true) {
        // 从服务器接收消息
        bytes_received = recv(client_socket, buffer, MAX_BUFFER_SIZE, 0);
        if (bytes_received <= 0) {  // 如果接收失败或连接关闭
            break;  // 退出循环
        }
        buffer[bytes_received] = '\0';  // 将接收到的数据结尾设置为'\0'，形成字符串
        cout << buffer;  // 输出接收到的消息
    }
}

int main() {
    WSADATA wsaData;  // 定义WSADATA结构体，用于存储Winsock的初始化信息
    // 初始化Winsock库
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // 创建客户端Socket
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in server_addr;  // 定义服务器地址结构
    server_addr.sin_family = AF_INET;  // 设置地址族为IPv4
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);  // 将IP地址转换并赋值
    server_addr.sin_port = htons(SERVER_PORT);  // 将端口号转换为网络字节序

    // 尝试连接到服务器
    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "连接到服务器失败\n";  // 如果连接失败，输出错误信息
        closesocket(client_socket);  // 关闭Socket
        WSACleanup();  // 清理Winsock资源
        return 1;  // 返回错误码
    }

    string username;  // 定义用户名字符串
    cout << "输入用户名: ";  // 提示用户输入用户名
    getline(cin, username);  // 获取用户输入的用户名

    // 发送用户名到服务器
    send(client_socket, username.c_str(), username.size(), 0);

    // 启动一个线程用于接收消息
    thread(receive_messages, client_socket).detach();  // 线程分离，使其独立运行

    // 主线程用于发送消息
    string message;  // 定义消息字符串
    while (true) {
        getline(cin, message);  // 获取用户输入的消息
        // 检查是否输入退出命令
        if (message == "/exit" || message == "/退出") {
            break;  // 退出循环
        }
        // 发送消息到服务器
        send(client_socket, message.c_str(), message.size(), 0);
    }

    closesocket(client_socket);  // 关闭Socket
    WSACleanup();  // 清理Winsock资源
    return 0; 
}
