#include <iostream>               // 引入输入输出流库
#include <thread>                // 引入线程库
#include <vector>                // 引入向量库
#include <string>                // 引入字符串库
#include <mutex>                 // 引入互斥量库
#include <map>                   // 引入映射（字典）库
#include <algorithm>             // 引入算法库（用于算法功能）
#include <winsock2.h>           // 引入Winsock库，用于网络编程

#pragma comment(lib, "ws2_32.lib") // 链接Winsock库

#define MAX_BUFFER_SIZE 1024    // 定义最大缓冲区大小
#define SERVER_PORT 8888        // 定义服务器端口

using namespace std;

// 全局变量
vector<SOCKET> clients;             // 存储所有已连接客户端的Socket
map<SOCKET, string> client_names;   // 存储客户端Socket与用户名的映射
mutex clients_mutex;                // 互斥量，用于保护客户端列表的访问

// 广播消息给所有连接的客户端
void broadcast(const string& message) {
    lock_guard<mutex> lock(clients_mutex);  // 自动加锁，保护对clients的访问
    for (SOCKET client : clients) {          // 遍历所有客户端Socket
        send(client, message.c_str(), message.size(), 0); // 发送消息
    }
}

// 广播当前在线人数给所有客户端
void broadcast_user_count() {
    string count_message = "当前在线人数: " + to_string(clients.size()) + "\n"; // 生成在线人数消息
    broadcast(count_message); // 广播消息
    cout << count_message; // 控制台输出在线人数
}

// 处理与单个客户端的交互
void handle_client(SOCKET client_socket) {
    char buffer[MAX_BUFFER_SIZE]; // 用于接收消息的缓冲区
    int bytes_received; // 实际接收到的字节数

    // 获取用户名
    bytes_received = recv(client_socket, buffer, MAX_BUFFER_SIZE, 0); // 接收用户名
    if (bytes_received <= 0) { // 如果接收失败
        closesocket(client_socket); // 关闭Socket
        return; // 退出函数
    }
    buffer[bytes_received] = '\0'; // 确保字符串以null结尾
    string username = buffer; // 保存用户名

    {
        lock_guard<mutex> lock(clients_mutex); // 自动加锁
        client_names[client_socket] = username; // 存储用户名
        clients.push_back(client_socket); // 将客户端Socket添加到列表
    }
    
    // 通知其他用户新用户加入
    string join_message = username + " 进入聊天室\n"; // 生成加入消息
    broadcast(join_message); // 广播消息
    cout << join_message; // 控制台输出加入消息
    broadcast_user_count(); // 显示在线人数
    
    while (true) { // 持续接收消息
        bytes_received = recv(client_socket, buffer, MAX_BUFFER_SIZE, 0); // 接收消息
        if (bytes_received <= 0) { // 如果接收失败，退出循环
            break;
        }
        buffer[bytes_received] = '\0'; // 确保字符串以null结尾
        string message = username + ": " + buffer; // 生成用户消息

        // 确保每条消息的末尾有换行符
        if (message.back() != '\n') { // 如果没有换行符
            message += "\n"; // 添加换行符
        }

        broadcast(message); // 广播消息
        cout << message; // 控制台输出消息
    }

    // 用户离开
    {
        lock_guard<mutex> lock(clients_mutex); // 自动加锁
        clients.erase(remove(clients.begin(), clients.end(), client_socket), clients.end()); // 移除离开的客户端
        client_names.erase(client_socket); // 移除用户名
    }
    string leave_message = username + " 退出聊天室\n"; // 生成离开消息
    broadcast(leave_message); // 广播消息
    cout << leave_message; // 控制台输出离开消息
    broadcast_user_count(); // 显示在线人数

    closesocket(client_socket); // 关闭与该客户端的Socket
}

int main() {
    WSADATA wsaData; // Winsock数据结构
    // 初始化Winsock库，指定版本为2.2
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // 创建一个TCP类型的Socket
    //(IPv4,TCP流式Socket,通信协议)
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in server_addr; // 定义服务器地址结构
    server_addr.sin_family = AF_INET; // 使用IPv4地址

    server_addr.sin_addr.s_addr = INADDR_ANY; // 允许接受任意IP地址的连接
    server_addr.sin_port = htons(SERVER_PORT); // 转换端口号为网络字节序

    // 将Socket与指定的地址和端口绑定
    bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr));
    // 将Socket设置为被动监听状态，等待客户端连接。
    listen(server_socket, SOMAXCONN);//SOMAXCONN最大连接请求排队数

    cout << "服务器启动，等待客户端连接...\n"; // 控制台输出服务器启动信息
    // 主循环，持续接受客户端连接
    while (true) {
        // 接受来自客户端的连接请求，返回一个新的Socket用于通信
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);//(监听的Socket描述符,客户端的地址信息,可选参数)
        // 检查连接是否成功
        if (client_socket != INVALID_SOCKET) {
            // 为每个连接的客户端创建一个新线程处理
            thread(handle_client, client_socket).detach(); // 启动新线程处理客户端
        }
    }

    // 关闭服务器Socket（这行代码在无限循环中实际上不会被执行）
    closesocket(server_socket); // 关闭服务器Socket
    // 清理Winsock资源
    WSACleanup(); // 清理Winsock资源
    return 0; 
}
