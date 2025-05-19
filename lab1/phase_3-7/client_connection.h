#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

// 前向声明
class ClientApp;

/**
 * @brief 处理客户端与服务器之间的网络连接
 */
class ClientConnection
{
private:
    SOCKET m_clientSocket;
    bool m_isConnected;
    ClientApp *m_app;

public:
    /**
     * @brief 构造函数
     * @param app 客户端应用指针
     */
    ClientConnection(ClientApp *app);

    /**
     * @brief 析构函数
     */
    ~ClientConnection();

    /**
     * @brief 连接到服务器
     * @return 是否成功连接
     */
    bool connectToServer();

    /**
     * @brief 向服务器发送请求并接收响应
     * @param request 请求字符串
     * @param response 响应字符串的引用
     * @return 是否成功发送并接收
     */
    bool sendRequest(const std::string &request, std::string &response);

    /**
     * @brief 断开与服务器的连接
     */
    void disconnect(); /**
                        * @brief 检查是否已连接
                        * @return 是否已连接
                        */
    bool isConnected() const;
};

#endif // CLIENT_CONNECTION_H
