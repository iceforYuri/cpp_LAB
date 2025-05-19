#pragma once
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <limits>
#include <sstream>
#include <iomanip>

class ClientApp {
private:
    SOCKET clientSocket;
    std::string currentUsername;
    std::string currentUserType;
    bool isLoggedIn;

public:
    // 构造函数和析构函数
    ClientApp();
    ~ClientApp();

    // 主要功能
    int run();
    
    // 网络相关方法
    bool connectToServer();
    bool sendRequest(const std::string &request, std::string &response) const;
    
    // 菜单相关方法
    int showMainMenu() const;
    int showCustomerMenu() const;
    int showSellerMenu() const;
    void clearInputBuffer() const;
    
    // 用户相关操作
    void handleLogin();
    void handleRegister();
    void handleLogout();
    void handleViewUserInfo();
    void handleChangePassword();
    void handleCheckBalance();
    void handleDeposit();
    
    // 商城相关操作
    void handleEnterStore();
    void handleBrowseProducts();
    void handleProductDetail();
    void handleSearchProducts();
    void handleDirectPurchase();
    
    // 购物车相关操作
    void handleViewCart();
    void handleAddToCart();
    void handleCheckout();
    
    // 商家相关操作
    void handleSellerInfo();
    void handleManageProducts();
    void handleCheckIncome();
};
