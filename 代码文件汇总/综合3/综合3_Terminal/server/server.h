#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>
#include <set>
#include <algorithm>
#include <chrono>

#include "../user/user.h"
#include "../store/store.h"
#include "../order/order.h"
#include "../ordermanager/ordermanager.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

class Server
{
private: // 成员变量
    SOCKET serverSocket;
    std::vector<std::thread> clientThreads;
    mutable std::mutex usersMutex;         // 使用mutable允许在const成员函数中修改
    mutable std::mutex storeMutex;         // 使用mutable允许在const成员函数中修改
    mutable std::mutex loggedInUsersMutex; // 使用mutable允许在const成员函数中修改
    std::atomic<bool> serverRunning;
    std::set<std::string> loggedInUsers;

    // 文件路径常量
    const string USER_FILE;
    const string STORE_FILE;
    const string CART_FILE;
    const string ORDER_DIR;

    vector<User *> users;
    Store store;
    OrderManager orderManager;

    // 处理客户端连接的成员函数
    void handleClient(SOCKET clientSocket);

    // 解析和处理各种客户端请求的辅助函数
    std::string processLoginRequest(const std::string &request);
    std::string processRegisterRequest(const std::string &request);
    std::string processGetProductsRequest(const std::string &request) const;
    std::string processGetProductDetailRequest(const std::string &request) const;
    std::string processAddToCartRequest(const std::string &request);
    std::string processGetCartRequest(const std::string &request);
    std::string processCheckoutRequest(const std::string &request);
    std::string processGetSellerInfoRequest(const std::string &request) const;
    std::string processChangePasswordRequest(const std::string &request);
    std::string processGetSellerProductsRequest(const std::string &request) const;
    std::string processAddBookRequest(const std::string &request);
    std::string processAddClothingRequest(const std::string &request);
    std::string processAddFoodRequest(const std::string &request);
    std::string processAddGenericRequest(const std::string &request);
    std::string processUpdateProductPriceRequest(const std::string &request);
    std::string processUpdateProductQuantityRequest(const std::string &request);
    std::string processSetProductDiscountRequest(const std::string &request);
    std::string processSetCategoryDiscountRequest(const std::string &request);
    std::string processLogoutRequest(const std::string &request);
    std::string processCheckBalanceRequest(const std::string &request) const;
    std::string processGetUserInfoRequest(const std::string &request) const;
    std::string processDepositRequest(const std::string &request);
    std::string processSearchProductsRequest(const std::string &request) const;
    std::string processDirectPurchaseRequest(const std::string &request);
    std::string processUpdateCartItemRequest(const std::string &request);
    std::string processRemoveFromCartRequest(const std::string &request);

public:
    // 构造函数
    Server(const std::string &userFile = "./user/users.txt",
           const std::string &storeFile = "./store",
           const std::string &cartFile = "./user/carts",
           const std::string &orderDir = "./order/orders");

    // 析构函数
    ~Server();

    // 服务器初始化和启动
    bool initialize();
    void start();
    void stop();

    // 获取服务器状态
    bool isRunning() const;
};

#endif // SERVER_H