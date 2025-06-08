#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <memory>
#include <map>
#include <signal.h>
#include "../network/socket.h"
#include "../store/store.h"
#include "../ordermanager/ordermanager.h"
#include "../user/user.h"
#include "request_handler.h"

// 全局变量，用于优雅地关闭服务器
volatile bool running = true;

// 信号处理函数
void signalHandler(int signum)
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    running = false;
}

// 处理客户端连接的线程函数
void handleClient(SOCKET clientSocket, const std::string &clientIP, RequestHandler &handler)
{
    ConnectionHandler connection(clientSocket, clientIP);

    std::cout << "Client connected: " << clientIP << std::endl;

    while (running)
    {
        // 接收客户端请求
        std::string request = connection.receiveRequest();
        if (request.empty())
        {
            break; // 客户端断开连接
        }

        // 处理请求并发送响应
        std::string response = handler.handleRequest(request, clientIP);
        if (!connection.sendResponse(response))
        {
            break; // 发送响应失败
        }
    }

    std::cout << "Client disconnected: " << clientIP << std::endl;
    connection.close();
}

int main()
{
    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // 创建应用程序实例
    Store store;
    OrderManager orderManager;
    std::map<std::string, std::shared_ptr<User>> users;

    // 初始化数据
    store.loadProducts("products.json");

    // 创建请求处理器
    RequestHandler requestHandler(store, orderManager, users);

    // 加载用户数据
    requestHandler.loadUserData();

    // 创建服务器socket
    ServerSocket serverSocket("0.0.0.0", 8888);
    if (!serverSocket.isInitialized())
    {
        std::cerr << "Failed to initialize server socket: " << serverSocket.getLastError() << std::endl;
        return 1;
    }

    // 绑定并开始监听
    if (!serverSocket.bindAndListen())
    {
        std::cerr << "Failed to bind and listen: " << serverSocket.getLastError() << std::endl;
        return 1;
    }

    std::cout << "Server started on port 8888" << std::endl;

    // 启动订单处理线程
    orderManager.startProcessing();

    std::vector<std::thread> clientThreads;

    // 主循环接受客户端连接
    while (running)
    {
        std::string clientIP;
        SOCKET clientSocket = serverSocket.acceptClient(clientIP);

        if (clientSocket != INVALID_SOCKET)
        {
            // 为每个客户端创建一个新的线程
            clientThreads.emplace_back(handleClient, clientSocket, clientIP, std::ref(requestHandler));
        }
    }

    std::cout << "Server shutting down..." << std::endl;

    // 关闭服务器socket
    serverSocket.close();

    // 停止订单处理
    orderManager.stopProcessing();

    // 等待所有客户端线程结束
    for (auto &thread : clientThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    // 保存数据
    store.saveProducts("products.json");
    requestHandler.saveUserData();

    std::cout << "Server shutdown complete" << std::endl;

    return 0;
}
