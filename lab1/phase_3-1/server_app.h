#pragma once
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

#include "user/user.h"
#include "store/store.h"
#include "order/order.h"
#include "ordermanager/ordermanager.h"

// 前向声明
class User;
class Store;
class OrderManager;

class ServerApp {
private:
    // 成员变量
    SOCKET serverSocket;
    std::vector<User*> users;
    Store store;
    OrderManager& orderManager;
    std::mutex usersMutex;
    std::mutex storeMutex;
    std::mutex loggedInUsersMutex;
    std::atomic<bool> serverRunning;
    std::set<std::string> loggedInUsers;
    std::vector<std::thread> clientThreads;

    // 文件路径常量
    const std::string USER_FILE;
    const std::string STORE_FILE;
    const std::string CART_FILE;
    const std::string ORDER_DIR;

public:
    // 构造函数和析构函数
    ServerApp();
    ~ServerApp();

    // 主要功能
    bool initialize();
    void run();
    void shutdown();
    
    // 客户端处理
    void handleClient(SOCKET clientSocket);
    
    // 用户管理功能
    void loadUsers();
    void saveUsers() const;
    User* loginUser(const std::string& username, const std::string& password) const;
    bool logoutUser(const std::string& username);
    bool isUserLoggedIn(const std::string& username) const;
    bool registerUser(const std::string& username, const std::string& password, const std::string& type);
    
    // 商品管理功能
    void loadStore();
    void saveStore() const;
    
    // 处理请求的方法
    std::string handleLoginRequest(const std::string& username, const std::string& password);
    std::string handleRegisterRequest(const std::string& username, const std::string& password, const std::string& type);
    std::string handleLogoutRequest(const std::string& username);
    std::string handleGetProductsRequest() const;
    std::string handleGetProductDetailRequest(const std::string& productName, const std::string& sellerUsername) const;
    std::string handleSearchProductsRequest(const std::string& searchType, const std::string& searchTerm) const;
    std::string handleSearchProductsByPriceRequest(double minPrice, double maxPrice) const;
    std::string handleGetUserInfoRequest(const std::string& username) const;
    std::string handleChangePasswordRequest(const std::string& username, const std::string& oldPassword, const std::string& newPassword);
    std::string handleCheckBalanceRequest(const std::string& username) const;
    std::string handleDepositRequest(const std::string& username, double amount);
    std::string handleDirectPurchaseRequest(const std::string& username, const std::string& productName, const std::string& sellerName, int quantity);
    std::string handleViewCartRequest(const std::string& username) const;
    std::string handleAddToCartRequest(const std::string& username, const std::string& productName, const std::string& sellerName, int quantity);
    std::string handleCheckoutRequest(const std::string& username);
    std::string handleSellerInfoRequest(const std::string& username) const;
    std::string handleCheckIncomeRequest(const std::string& username) const;
    std::string handleAddProductRequest(const std::string& sellerUsername, const std::string& productInfo);
    std::string handleUpdateProductRequest(const std::string& sellerUsername, const std::string& productName, const std::string& updateInfo);
    std::string handleRemoveProductRequest(const std::string& sellerUsername, const std::string& productName);
};
