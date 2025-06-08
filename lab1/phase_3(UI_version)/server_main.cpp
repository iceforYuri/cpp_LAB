#include <iostream>
#include <string>
#include <signal.h>
#include "network/server.h"
#include <windows.h>

// 全局服务器指针，用于信号处理
NetworkServer *g_server = nullptr;

// 信号处理函数
void signalHandler(int signal)
{
    std::cout << "\n收到退出信号，正在关闭服务器..." << std::endl;
    if (g_server)
    {
        g_server->stop();
    }
}

int main()
{
    // 设置控制台输出编码为UTF-8
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    std::cout << "=== 电子商城服务端 ===" << std::endl;

    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // 获取服务器配置
    int port = 8888;
    std::cout << "服务器将在端口 " << port << " 上启动" << std::endl;

    // 创建服务器
    NetworkServer server(port);
    g_server = &server;

    // 启动服务器
    if (!server.start())
    {
        std::cerr << "服务器启动失败" << std::endl;
        return 1;
    }

    std::cout << "服务器已启动，正在监听端口 " << port << std::endl;
    std::cout << "按 Ctrl+C 停止服务器" << std::endl;

    // 服务器主循环
    while (server.isServerRunning())
    {
        // 服务器在后台运行，这里可以添加管理命令
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "服务器已停止" << std::endl;
    return 0;
}
