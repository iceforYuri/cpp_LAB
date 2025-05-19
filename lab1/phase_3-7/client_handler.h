#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <vector>
#include <mutex>
#include <sstream>

#include "user/user.h"
#include "store/store.h"
#include "order/order.h"
#include "ordermanager/ordermanager.h"
#include "request_processor.h"

// 前向声明
class ServerApp;
class RequestProcessor;

/**
 * @brief 处理客户端连接和请求
 */
class ClientHandler
{
private:
    SOCKET m_clientSocket;
    ServerApp *m_server;
    std::string m_currentConnectedUser;
    bool m_clientConnected;
    RequestProcessor *m_processor;

public:
    /**
     * @brief 构造函数
     * @param clientSocket 客户端套接字
     * @param server 服务器应用指针
     */
    ClientHandler(SOCKET clientSocket, ServerApp *server);

    /**
     * @brief 析构函数
     */
    ~ClientHandler();

    /**
     * @brief 处理客户端请求
     */
    void handleClient();

    /**
     * @brief 处理单个请求
     * @param request 请求字符串
     * @return 响应字符串
     */
    std::string processRequest(const std::string &request);

    /**
     * @brief 发送响应给客户端
     * @param response 响应字符串
     * @return 是否发送成功
     */
    bool sendResponse(const std::string &response);

    /**
     * @brief 断开客户端连接
     */
    void disconnectClient();

    /**
     * @brief 获取当前连接的用户名
     * @return 用户名
     */
    std::string getCurrentConnectedUser() const;

    /**
     * @brief 设置当前连接的用户名
     * @param username 用户名
     */
    void setCurrentConnectedUser(const std::string &username);

    /**
     * @brief 客户端是否连接
     * @return 是否连接
     */
    bool isClientConnected() const;

    /**
     * @brief 获取服务器应用指针
     * @return 服务器应用指针
     */
    ServerApp *getServer() const;

    /**
     * @brief 获取客户端套接字
     * @return 客户端套接字
     */
    SOCKET getClientSocket() const;
};

#endif // CLIENT_HANDLER_H
