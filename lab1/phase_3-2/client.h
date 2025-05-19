#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <sstream>
#include <limits>
#include <iomanip>

#pragma comment(lib, "ws2_32.lib")

class Client {
private:
    SOCKET clientSocket;
    std::string currentUsername;
    std::string currentUserType;
    bool isLoggedIn;

    // 辅助方法
    void clearInputBuffer() const;

public:
    // 构造函数与析构函数
    Client();
    ~Client();

    // 连接与通信
    bool connectToServer();
    bool sendRequest(const std::string &request, std::string &response) const;

    // 菜单显示
    int showMainMenu() const;
    int showCustomerMenu() const;
    int showSellerMenu() const;

    // 用户操作处理
    void handleLogin();
    void handleRegister();
    void handleLogout();
    
    // 商品浏览与购物车操作
    void handleBrowseProducts() const;
    void handleProductDetail() const;
    void handleViewCart() const;
    void handleAddToCart() const;
    void handleCheckout() const;
    void handleSearchProducts() const;
    void handleDirectPurchase() const;
    
    // 用户信息管理
    void handleViewUserInfo() const;
    void handleCheckBalance() const;
    void handleDeposit() const;
    void handleEnterStore();
    
    // 商家特定功能
    void handleSellerInfo() const;
    void handleChangePassword() const;
    void handleManageProducts() const;
    void handleCheckIncome() const;

    // 运行客户端主循环
    int run();
};

#endif // CLIENT_H
