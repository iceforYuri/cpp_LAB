#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>
#include <set>
#include <algorithm>
#include <chrono>

#include "user/user.h"
#include "store/store.h"
#include "order/order.h"
#include "ordermanager/ordermanager.h"

#pragma comment(lib, "ws2_32.lib")

class Server {
private:
    // 服务器socket
    SOCKET serverSocket;
    
    // 互斥锁
    std::mutex usersMutex;
    std::mutex storeMutex;
    std::mutex loggedInUsersMutex;
    
    // 控制标志
    std::atomic<bool> serverRunning;
    
    // 已登录用户集合
    std::set<std::string> loggedInUsers;
    
    // 资源管理
    std::vector<User *> users;
    Store store;
    OrderManager orderManager;
    
    // 文件路径常量
    const std::string USER_FILE;
    const std::string STORE_FILE;
    const std::string CART_FILE;
    const std::string ORDER_DIR;
    
    // 客户端处理
    void handleClient(SOCKET clientSocket);
    
    // 请求处理辅助方法
    std::string handleLoginRequest(const std::string &request, std::string &currentConnectedUser);
    std::string handleRegisterRequest(const std::string &request);
    std::string handleGetProductsRequest() const;
    std::string handleGetProductDetailRequest(const std::string &request) const;
    std::string handleAddToCartRequest(const std::string &request);
    std::string handleGetCartRequest(const std::string &request) const;
    std::string handleCheckoutRequest(const std::string &request);
    std::string handleLogoutRequest(const std::string &request, std::string &currentConnectedUser);
    std::string handleGetUserInfoRequest(const std::string &request) const;
    std::string handleCheckBalanceRequest(const std::string &request) const;
    std::string handleDepositRequest(const std::string &request);
    // 其他请求处理方法...

public:
    // 构造函数与析构函数
    Server();
    ~Server();
    
    // 服务器初始化与启动
    bool initialize();
    bool start();
    void stop();
    
    // 主循环
    void run();
};

#endif // SERVER_H
