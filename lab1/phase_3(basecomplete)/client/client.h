#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <limits>
#include <iomanip>

#pragma comment(lib, "ws2_32.lib")

class Client
{
private:
    SOCKET clientSocket;
    std::string currentUsername;
    std::string currentUserType;
    bool isLoggedIn; // 辅助函数
    bool connectToServer();
    bool sendRequest(const std::string &request, std::string &response);
    void clearInputBuffer() const;

    // 菜单显示函数
    int showMainMenu() const;
    int showCustomerMenu() const;
    int showSellerMenu() const;

    // 用户操作处理函数
    void handleLogin();
    void handleRegister();
    void handleBrowseProducts();
    void handleProductDetail();
    void handleViewCart();
    void handleAddToCart();
    void handleCheckout();
    void handleLogout();
    void handleViewUserInfo();
    void handleCheckBalance();
    void handleDeposit();
    void handleEnterStore();
    void handleSearchProducts();
    void handleDirectPurchase();

    // 商家相关函数
    void handleSellerInfo();
    void handleChangePassword();
    void handleManageProducts();
    void handleCheckIncome();

public:
    Client();
    ~Client();

    bool initialize();
    void run();
    void cleanup();
};

#endif // CLIENT_H
