#include <iostream>
#include <csignal>
#include "server/server.h"

// 全局变量，用于信号处理
Server *g_server = nullptr;

// 信号处理函数，用于优雅地关闭服务器
void signalHandler(int signal)
{
    std::cout << "接收到信号: " << signal << std::endl;
    if (g_server)
    {
        std::cout << "正在停止服务器..." << std::endl;
        g_server->stop();
    }
}

int main()
{
    // 设置信号处理
    signal(SIGINT, signalHandler);  // 处理Ctrl+C
    signal(SIGTERM, signalHandler); // 处理终止信号

    // 创建服务器实例
    Server server;
    g_server = &server;

    // 初始化服务器
    if (!server.initialize())
    {
        std::cerr << "服务器初始化失败！" << std::endl;
        return 1;
    }

    // 启动服务器
    server.start();

    std::cout << "服务器已关闭。" << std::endl;
    return 0;
}