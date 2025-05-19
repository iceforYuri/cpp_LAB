#ifndef CLIENT_APP_H
#define CLIENT_APP_H

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <limits>
#include <iomanip>

#include "client_connection.h"
#include "client_ui.h"

#pragma comment(lib, "ws2_32.lib")

// 前向声明
class ClientConnection;
class ClientUI;

/**
 * @brief 客户端应用主类，管理整个客户端应用的生命周期和状态
 */
class ClientApp
{
private:
    // 会话状态
    std::string m_currentUsername;
    std::string m_currentUserType;
    bool m_isLoggedIn;

    // 组件
    ClientConnection *m_connection;
    ClientUI *m_ui;

public:
    /**
     * @brief 构造函数
     */
    ClientApp();

    /**
     * @brief 析构函数
     */
    ~ClientApp();

    /**
     * @brief 运行应用程序
     * @return 退出码
     */
    int run();

    /**
     * @brief 获取当前用户名
     * @return 当前用户名
     */
    std::string getCurrentUsername() const;

    /**
     * @brief 获取当前用户类型
     * @return 当前用户类型
     */
    std::string getCurrentUserType() const;

    /**
     * @brief 检查是否已登录
     * @return 是否已登录
     */
    bool isLoggedIn() const;

    /**
     * @brief 设置当前用户名
     * @param username 用户名
     */
    void setCurrentUsername(const std::string &username);

    /**
     * @brief 设置当前用户类型
     * @param userType 用户类型
     */
    void setCurrentUserType(const std::string &userType);

    /**
     * @brief 设置登录状态
     * @param isLoggedIn 是否已登录
     */
    void setLoggedIn(bool isLoggedIn); /**
                                        * @brief 获取连接对象
                                        * @return 连接对象
                                        */
    ClientConnection *getConnection() const;

    // 处理各种功能的方法
    void handleLogin();
    void handleRegister();
    void handleBrowseProducts();
    void handleProductDetail();
    void handleViewCart();
    void handleAddToCart();
    void handleCheckout(const std::vector<std::pair<std::string, std::string>> &cartItems,
                        const std::vector<int> &quantities,
                        const std::vector<double> &prices,
                        double totalAmount);
    void handleLogout();
    void handleViewUserInfo();
    void handleCheckBalance();
    void handleDeposit();
    void handleEnterStore();
    void handleSearchProducts();
    void handleDirectPurchase();
    void handleSellerInfo();
    void handleChangePassword();
    void handleManageProducts();
    void handleCheckIncome();

    // 商品管理相关辅助方法
    void viewMyProducts();
    void addNewProduct();
    void modifyProduct();
    void removeProduct();
};

#endif // CLIENT_APP_H
