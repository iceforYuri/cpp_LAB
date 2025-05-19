#include "client_app.h"
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

using namespace std;

// 构造函数
ClientApp::ClientApp() : clientSocket(INVALID_SOCKET), isLoggedIn(false) {
    // 初始化Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup失败: " << WSAGetLastError() << std::endl;
        exit(1);
    }
}

// 析构函数
ClientApp::~ClientApp() {
    // 关闭socket连接
    if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
    }
    
    // 清理Winsock
    WSACleanup();
}

// 主运行函数
int ClientApp::run() {
    // 连接到服务器
    if (!connectToServer()) {
        return 1;
    }

    int choice;
    bool running = true;

    while (running) {
        if (!isLoggedIn) {
            // 显示主菜单
            choice = showMainMenu();

            switch (choice) {
                case 1:
                    handleLogin();
                    break;
                case 2:
                    handleRegister();
                    break;
                case 0:
                    running = false;
                    break;
                default:
                    break;
            }
        } else {
            // 根据用户类型显示不同的菜单
            if (currentUserType == "customer") {
                choice = showCustomerMenu();

                switch (choice) {
                    case 1:
                        handleViewUserInfo();
                        break;
                    case 2:
                        handleChangePassword();
                        break;
                    case 3:
                        handleCheckBalance();
                        break;
                    case 4:
                        handleDeposit();
                        break;
                    case 5:
                        handleEnterStore();
                        break;
                    case -1: // 退出登录
                        handleLogout();
                        break;
                    case 0: // 退出程序
                        running = false;
                        break;
                    default:
                        break;
                }
            } else if (currentUserType == "seller") {
                choice = showSellerMenu();

                switch (choice) {
                    case 1:
                        handleSellerInfo();
                        break;
                    case 2:
                        handleChangePassword();
                        break;
                    case 3:
                        handleManageProducts();
                        break;
                    case 4:
                        handleCheckIncome();
                        break;
                    case -1: // 退出登录
                        handleLogout();
                        break;
                    case 0: // 退出程序
                        running = false;
                        break;
                    default:
                        break;
                }
            }
        }
    }

    return 0;
}

// 连接到服务器
bool ClientApp::connectToServer() {
    // 创建客户端socket
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "创建socket失败: " << WSAGetLastError() << std::endl;
        return false;
    }

    // 设置服务器地址
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    // 默认连接到本地服务器，可以根据需要修改
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
    serverAddr.sin_port = htons(8888); // 使用8888端口

    // 连接到服务器
    if (connect(clientSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "连接失败: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        return false;
    }

    std::cout << "已连接到服务器。" << std::endl;
    return true;
}

// 发送请求并接收响应
bool ClientApp::sendRequest(const std::string &request, std::string &response) const {
    // 发送请求
    if (send(clientSocket, request.c_str(), request.size(), 0) == SOCKET_ERROR) {
        std::cerr << "发送失败: " << WSAGetLastError() << std::endl;
        return false;
    }

    // 接收响应
    char buffer[4096] = {0};
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        std::cerr << "接收失败或连接关闭: " << WSAGetLastError() << std::endl;
        return false;
    }

    response = std::string(buffer, bytesReceived);
    return true;
}

// 清除输入缓冲区
void ClientApp::clearInputBuffer() const {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// 显示主菜单
int ClientApp::showMainMenu() const {
    int choice;

    std::cout << "\n===== 欢迎使用网络电子商城系统 =====\n";
    std::cout << "1. 登录\n";
    std::cout << "2. 注册\n";
    std::cout << "0. 退出\n";
    std::cout << "请选择: ";

    std::cin >> choice;
    clearInputBuffer();

    if (choice < 0 || choice > 2) {
        std::cout << "无效选择，请重试。\n";
        return 1; // 默认返回继续
    }

    return choice;
}

// 显示消费者菜单
int ClientApp::showCustomerMenu() const {
    int choice;

    std::cout << "\n\n\n--- 消费者菜单 (" << currentUsername << ") ---\n";
    std::cout << "1. 查看个人信息\n";
    std::cout << "2. 修改密码\n";
    std::cout << "3. 查询余额\n";
    std::cout << "4. 充值\n";
    std::cout << "5. 进入商城\n";
    std::cout << "6. 退出登录\n";
    std::cout << "0. 退出程序\n";
    std::cout << "请选择一个选项: ";

    std::cin >> choice;
    clearInputBuffer();

    if (choice == 6)
        return -1; // 退出登录
    if (choice == 0)
        return 0; // 退出程序
    if (choice < 0 || choice > 6) {
        std::cout << "无效选择，请重试。\n";
        return 1; // 默认返回继续
    }

    return choice;
}

// 显示商家菜单
int ClientApp::showSellerMenu() const {
    int choice;

    std::cout << "\n===== 商家菜单 (" << currentUsername << ") =====\n";
    std::cout << "1. 查看商户信息\n";
    std::cout << "2. 修改密码\n";
    std::cout << "3. 进入商城 (管理)\n";
    std::cout << "4. 查看收入\n";
    std::cout << "5. 退出登录\n";
    std::cout << "0. 退出程序\n";
    std::cout << "请选择: ";

    std::cin >> choice;
    clearInputBuffer();

    if (choice == 5)
        return -1; // 退出登录
    if (choice == 0)
        return 0; // 退出程序
    if (choice < 0 || choice > 5) {
        std::cout << "无效选择，请重试。\n";
        return 1; // 默认返回继续
    }

    return choice;
}

// 处理登录功能
void ClientApp::handleLogin() {
    std::string username, password;

    std::cout << "\n===== 用户登录 =====\n";
    std::cout << "用户名: ";
    std::getline(std::cin, username);
    std::cout << "密码: ";
    std::getline(std::cin, password);

    std::string request = "LOGIN|" + username + "|" + password;
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, message, userType;

        std::getline(iss, status, '|');

        if (status == "LOGIN_SUCCESS") {
            std::getline(iss, message, '|');
            std::getline(iss, userType, '|');

            isLoggedIn = true;
            currentUsername = message;
            currentUserType = userType;

            std::cout << "登录成功！欢迎 " << currentUsername << "(" << currentUserType << ")" << std::endl;
        } else {
            std::getline(iss, message, '|');
            std::cout << "登录失败: " << message << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }
}

// 处理注册功能
void ClientApp::handleRegister() {
    std::string username, password, confirmPassword, userType;
    int typeChoice;

    std::cout << "\n===== 用户注册 =====\n";
    std::cout << "用户名: ";
    std::getline(std::cin, username);
    std::cout << "密码: ";
    std::getline(std::cin, password);
    std::cout << "确认密码: ";
    std::getline(std::cin, confirmPassword);

    if (password != confirmPassword) {
        std::cout << "两次输入的密码不一致，请重试。" << std::endl;
        return;
    }

    std::cout << "请选择用户类型:\n";
    std::cout << "1. 消费者\n";
    std::cout << "2. 商家\n";
    std::cout << "请选择: ";
    std::cin >> typeChoice;
    clearInputBuffer();

    if (typeChoice == 1) {
        userType = "customer";
    } else if (typeChoice == 2) {
        userType = "seller";
    } else {
        std::cout << "无效选择，注册失败。" << std::endl;
        return;
    }

    std::string request = "REGISTER|" + username + "|" + password + "|" + userType;
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, message;

        std::getline(iss, status, '|');
        std::getline(iss, message, '|');

        if (status == "REGISTER_SUCCESS") {
            std::cout << "注册成功！新用户: " << message << std::endl;
        } else {
            std::cout << "注册失败: " << message << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }
}

// 处理登出功能
void ClientApp::handleLogout() {
    std::string request = "LOGOUT|" + currentUsername;
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, message;

        std::getline(iss, status, '|');
        std::getline(iss, message, '|');

        if (status == "LOGOUT_SUCCESS") {
            isLoggedIn = false;
            currentUsername = "";
            currentUserType = "";
            std::cout << "已成功登出。" << std::endl;
        } else {
            std::cout << "登出失败: " << message << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败，已强制登出。" << std::endl;
        isLoggedIn = false;
        currentUsername = "";
        currentUserType = "";
    }
}

// 处理查看用户信息功能
void ClientApp::handleViewUserInfo() {
    std::string request = "GET_USER_INFO|" + currentUsername;
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, username, userType, balanceStr, creationTimeStr;

        std::getline(iss, status, '|');

        if (status == "USER_INFO") {
            std::getline(iss, username, '|');
            std::getline(iss, userType, '|');
            std::getline(iss, balanceStr, '|');
            std::getline(iss, creationTimeStr, '|');

            std::cout << "\n===== 用户信息 =====\n";
            std::cout << "用户名: " << username << std::endl;
            std::cout << "用户类型: " << userType << std::endl;
            std::cout << "账户余额: " << std::fixed << std::setprecision(2) << std::stod(balanceStr) << " 元" << std::endl;
            std::cout << "账户创建时间: " << creationTimeStr << std::endl;
        } else {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "获取用户信息失败: " << message << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

// 处理修改密码功能
void ClientApp::handleChangePassword() {
    std::string oldPassword, newPassword, confirmPassword;

    std::cout << "\n===== 修改密码 =====\n";
    std::cout << "当前用户: " << currentUsername << std::endl;
    std::cout << "旧密码: ";
    std::getline(std::cin, oldPassword);
    std::cout << "新密码: ";
    std::getline(std::cin, newPassword);
    std::cout << "确认新密码: ";
    std::getline(std::cin, confirmPassword);

    if (newPassword != confirmPassword) {
        std::cout << "两次输入的新密码不一致，请重试。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    std::string request = "CHANGE_PASSWORD|" + currentUsername + "|" + oldPassword + "|" + newPassword;
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, message;

        std::getline(iss, status, '|');
        std::getline(iss, message, '|');

        if (status == "PASSWORD_CHANGED") {
            std::cout << "密码修改成功！" << std::endl;
        } else {
            std::cout << "密码修改失败: " << message << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

// 处理查询余额功能
void ClientApp::handleCheckBalance() {
    std::string request = "CHECK_BALANCE|" + currentUsername;
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, balanceStr;

        std::getline(iss, status, '|');

        if (status == "BALANCE") {
            std::getline(iss, balanceStr, '|');
            double balance = std::stod(balanceStr);

            std::cout << "\n===== 账户余额 =====\n";
            std::cout << "用户: " << currentUsername << std::endl;
            std::cout << "当前余额: " << std::fixed << std::setprecision(2) << balance << " 元" << std::endl;
        } else {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "查询余额失败: " << message << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

// 处理充值功能
void ClientApp::handleDeposit() {
    double amount;

    std::cout << "\n===== 充值 =====\n";
    std::cout << "请输入充值金额: ";
    std::cin >> amount;
    clearInputBuffer();

    if (amount <= 0) {
        std::cout << "充值金额必须大于0。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    std::string request = "DEPOSIT|" + currentUsername + "|" + std::to_string(amount);
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, message, newBalanceStr;

        std::getline(iss, status, '|');

        if (status == "DEPOSIT_SUCCESS") {
            std::getline(iss, message, '|');
            std::getline(iss, newBalanceStr, '|');
            double newBalance = std::stod(newBalanceStr);

            std::cout << "充值成功！" << std::endl;
            std::cout << "充值金额: " << std::fixed << std::setprecision(2) << amount << " 元" << std::endl;
            std::cout << "当前余额: " << std::fixed << std::setprecision(2) << newBalance << " 元" << std::endl;
        } else {
            std::getline(iss, message, '|');
            std::cout << "充值失败: " << message << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

// 处理进入商城功能
void ClientApp::handleEnterStore() {
    int choice;
    bool inStore = true;

    while (inStore) {
        std::cout << "\n===== 电子商城 =====\n";
        std::cout << "1. 浏览所有商品\n";
        std::cout << "2. 查看商品详情\n";
        std::cout << "3. 搜索商品\n";
        std::cout << "4. 直接购买商品\n";
        
        if (currentUserType == "customer") {
            std::cout << "5. 查看购物车\n";
            std::cout << "6. 添加商品到购物车\n";
            std::cout << "7. 结算购物车\n";
        } else if (currentUserType == "seller") {
            std::cout << "5. 添加商品\n";
            std::cout << "6. 修改商品\n";
            std::cout << "7. 下架商品\n";
        }
        
        std::cout << "0. 返回上一级\n";
        std::cout << "请选择: ";
        
        std::cin >> choice;
        clearInputBuffer();

        if (choice == 0) {
            inStore = false;
            continue;
        }

        // 处理通用的商城选项
        switch (choice) {
            case 1:
                handleBrowseProducts();
                break;
            case 2:
                handleProductDetail();
                break;
            case 3:
                handleSearchProducts();
                break;
            case 4:
                handleDirectPurchase();
                break;
            default:
                break;
        }

        // 处理特定用户类型的选项
        if (currentUserType == "customer") {
            switch (choice) {
                case 5:
                    handleViewCart();
                    break;
                case 6:
                    handleAddToCart();
                    break;
                case 7:
                    handleCheckout();
                    break;
                default:
                    break;
            }
        } else if (currentUserType == "seller") {
            switch (choice) {
                case 5:
                    // 处理添加商品
                    break;
                case 6:
                    // 处理修改商品
                    break;
                case 7:
                    // 处理下架商品
                    break;
                default:
                    break;
            }
        }
    }
}

// 处理浏览所有商品功能
void ClientApp::handleBrowseProducts() {
    std::string request = "GET_PRODUCTS";
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, countStr;

        std::getline(iss, status, '|');

        if (status == "PRODUCTS") {
            std::getline(iss, countStr, '|');
            int count = std::stoi(countStr);

            std::cout << "\n===== 商品列表 =====\n";
            if (count == 0) {
                std::cout << "当前没有可用的商品。" << std::endl;
            } else {
                std::cout << "共找到 " << count << " 件商品:\n\n";
                std::cout << std::left << std::setw(20) << "商品名称"
                          << std::setw(15) << "类型"
                          << std::setw(10) << "价格"
                          << std::setw(10) << "库存"
                          << "商家" << std::endl;
                std::cout << std::string(70, '-') << std::endl;

                for (int i = 0; i < count; i++) {
                    std::string productInfo;
                    std::getline(iss, productInfo, '|');

                    std::istringstream productStream(productInfo);
                    std::string name, type, priceStr, quantityStr, seller;

                    std::getline(productStream, name, ',');
                    std::getline(productStream, type, ',');
                    std::getline(productStream, priceStr, ',');
                    std::getline(productStream, quantityStr, ',');
                    std::getline(productStream, seller, ',');

                    double price = std::stod(priceStr);
                    int quantity = std::stoi(quantityStr);

                    std::cout << std::left << std::setw(20) << name
                              << std::setw(15) << type
                              << std::setw(10) << std::fixed << std::setprecision(2) << price
                              << std::setw(10) << quantity
                              << seller << std::endl;
                }
            }
        } else {
            std::cout << "获取商品列表失败。" << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "\n按回车键继续...";
    std::cin.get();
}

// 处理查看商品详情功能
void ClientApp::handleProductDetail() {
    std::string productName, sellerName;

    std::cout << "\n===== 查看商品详情 =====\n";
    std::cout << "请输入商品名称: ";
    std::getline(std::cin, productName);
    std::cout << "请输入商家名称: ";
    std::getline(std::cin, sellerName);

    std::string request = "GET_PRODUCT_DETAIL|" + productName + "|" + sellerName;
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status;

        std::getline(iss, status, '|');

        if (status == "PRODUCT_DETAIL") {
            std::string name, type, description, priceStr, quantityStr, seller;
            std::string extraInfo1, extraInfo2;

            std::getline(iss, name, '|');
            std::getline(iss, type, '|');
            std::getline(iss, description, '|');
            std::getline(iss, priceStr, '|');
            std::getline(iss, quantityStr, '|');
            std::getline(iss, seller, '|');

            double price = std::stod(priceStr);
            int quantity = std::stoi(quantityStr);

            std::cout << "\n===== 商品详情 =====\n";
            std::cout << "商品名称: " << name << std::endl;
            std::cout << "商品类型: " << type << std::endl;
            std::cout << "商品描述: " << description << std::endl;
            std::cout << "价格: " << std::fixed << std::setprecision(2) << price << " 元" << std::endl;
            std::cout << "库存: " << quantity << std::endl;
            std::cout << "商家: " << seller << std::endl;

            // 处理特定类型商品的额外信息
            if (type == "Book") {
                // 书籍特有属性
                std::getline(iss, extraInfo1, '|'); // 作者
                std::getline(iss, extraInfo2, '|'); // ISBN
                std::cout << "作者: " << extraInfo1 << std::endl;
                std::cout << "ISBN: " << extraInfo2 << std::endl;
            } else if (type == "Clothing") {
                // 服装特有属性
                std::getline(iss, extraInfo1, '|'); // 尺码
                std::getline(iss, extraInfo2, '|'); // 材质
                std::cout << "尺码: " << extraInfo1 << std::endl;
                std::cout << "材质: " << extraInfo2 << std::endl;
            } else if (type == "Food") {
                // 食品特有属性
                std::getline(iss, extraInfo1, '|'); // 保质期
                std::cout << "保质期: " << extraInfo1 << std::endl;
            } else if (type == "Electronics") {
                // 电子产品特有属性
                std::getline(iss, extraInfo1, '|'); // 品牌
                std::getline(iss, extraInfo2, '|'); // 规格
                std::cout << "品牌: " << extraInfo1 << std::endl;
                std::cout << "规格: " << extraInfo2 << std::endl;
            }
        } else {
            std::string errorMsg;
            std::getline(iss, errorMsg, '|');
            std::cout << "获取商品详情失败: " << errorMsg << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "\n按回车键继续...";
    std::cin.get();
}

// 处理搜索商品功能
void ClientApp::handleSearchProducts() {
    std::string searchTerm;
    int searchType;

    std::cout << "\n===== 搜索商品 =====\n";
    std::cout << "选择搜索类型:\n";
    std::cout << "1. 按名称搜索\n";
    std::cout << "2. 按类型搜索\n";
    std::cout << "3. 按价格区间搜索\n";
    std::cout << "请选择: ";
    std::cin >> searchType;
    clearInputBuffer();

    std::string request;

    if (searchType == 1) {
        std::cout << "请输入商品名称关键词: ";
        std::getline(std::cin, searchTerm);
        request = "SEARCH_PRODUCTS|name|" + searchTerm;
    } else if (searchType == 2) {
        std::cout << "请输入商品类型 (Book/Clothing/Food/Electronics): ";
        std::getline(std::cin, searchTerm);
        request = "SEARCH_PRODUCTS|type|" + searchTerm;
    } else if (searchType == 3) {
        double minPrice, maxPrice;
        std::cout << "请输入最低价格: ";
        std::cin >> minPrice;
        std::cout << "请输入最高价格: ";
        std::cin >> maxPrice;
        clearInputBuffer();

        request = "SEARCH_PRODUCTS|price|" + std::to_string(minPrice) + "|" + std::to_string(maxPrice);
    } else {
        std::cout << "无效的选择。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, countStr;

        std::getline(iss, status, '|');

        if (status == "SEARCH_RESULTS") {
            std::getline(iss, countStr, '|');
            int count = std::stoi(countStr);

            std::cout << "\n===== 搜索结果 =====\n";
            if (count == 0) {
                std::cout << "没有找到匹配的商品。" << std::endl;
            } else {
                std::cout << "共找到 " << count << " 件商品:\n\n";
                std::cout << std::left << std::setw(20) << "商品名称"
                          << std::setw(15) << "类型"
                          << std::setw(10) << "价格"
                          << std::setw(10) << "库存"
                          << "商家" << std::endl;
                std::cout << std::string(70, '-') << std::endl;

                for (int i = 0; i < count; i++) {
                    std::string productInfo;
                    std::getline(iss, productInfo, '|');

                    std::istringstream productStream(productInfo);
                    std::string name, type, priceStr, quantityStr, seller;

                    std::getline(productStream, name, ',');
                    std::getline(productStream, type, ',');
                    std::getline(productStream, priceStr, ',');
                    std::getline(productStream, quantityStr, ',');
                    std::getline(productStream, seller, ',');

                    double price = std::stod(priceStr);
                    int quantity = std::stoi(quantityStr);

                    std::cout << std::left << std::setw(20) << name
                              << std::setw(15) << type
                              << std::setw(10) << std::fixed << std::setprecision(2) << price
                              << std::setw(10) << quantity
                              << seller << std::endl;
                }
            }
        } else {
            std::string errorMsg;
            std::getline(iss, errorMsg, '|');
            std::cout << "搜索失败: " << errorMsg << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "\n按回车键继续...";
    std::cin.get();
}

// 处理直接购买功能
void ClientApp::handleDirectPurchase() {
    if (currentUserType != "customer") {
        std::cout << "只有消费者可以进行购买操作。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    std::string productName, sellerName;
    int quantity;

    std::cout << "\n===== 直接购买商品 =====\n";
    std::cout << "请输入商品名称: ";
    std::getline(std::cin, productName);
    std::cout << "请输入商家名称: ";
    std::getline(std::cin, sellerName);
    std::cout << "请输入购买数量: ";
    std::cin >> quantity;
    clearInputBuffer();

    if (quantity <= 0) {
        std::cout << "购买数量必须大于0。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 先获取商品详情
    std::string detailRequest = "GET_PRODUCT_DETAIL|" + productName + "|" + sellerName;
    std::string detailResponse;

    if (!sendRequest(detailRequest, detailResponse)) {
        std::cout << "与服务器通信失败，无法获取商品详情。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    std::istringstream detailIss(detailResponse);
    std::string detailStatus;
    std::getline(detailIss, detailStatus, '|');

    if (detailStatus != "PRODUCT_DETAIL") {
        std::string errorMsg;
        std::getline(detailIss, errorMsg, '|');
        std::cout << "未找到商品: " << errorMsg << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 商品存在，获取价格和库存
    std::string name, type, description, priceStr, quantityStr, seller;
    std::getline(detailIss, name, '|');
    std::getline(detailIss, type, '|');
    std::getline(detailIss, description, '|');
    std::getline(detailIss, priceStr, '|');
    std::getline(detailIss, quantityStr, '|');
    std::getline(detailIss, seller, '|');

    double price = std::stod(priceStr);
    int stock = std::stoi(quantityStr);
    double totalCost = price * quantity;

    // 检查库存
    if (stock < quantity) {
        std::cout << "库存不足！当前库存: " << stock << std::endl;
        std::cout << "购买失败！按回车键继续...";
        std::cin.get();
        return;
    }

    // 获取当前余额
    std::string balanceRequest = "CHECK_BALANCE|" + currentUsername;
    std::string balanceResponse;
    double balance = 0.0;

    if (sendRequest(balanceRequest, balanceResponse)) {
        std::istringstream balanceIss(balanceResponse);
        std::string balanceStatus, balanceStr;
        std::getline(balanceIss, balanceStatus, '|');

        if (balanceStatus == "BALANCE") {
            std::getline(balanceIss, balanceStr, '|');
            balance = std::stod(balanceStr);
        }
    }

    if (balance < totalCost) {
        std::cout << "余额不足！当前余额: " << std::fixed << std::setprecision(2) << balance << " 元" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 显示订单信息并请求确认
    std::cout << "\n===== 订单确认 =====\n";
    std::cout << "商品: " << name << std::endl;
    std::cout << "类型: " << type << std::endl;
    std::cout << "单价: " << std::fixed << std::setprecision(2) << price << " 元" << std::endl;
    std::cout << "数量: " << quantity << std::endl;
    std::cout << "总价: " << std::fixed << std::setprecision(2) << totalCost << " 元" << std::endl;
    std::cout << "您的当前余额: " << std::fixed << std::setprecision(2) << balance << " 元" << std::endl;
    std::cout << "支付后余额将为: " << std::fixed << std::setprecision(2) << (balance - totalCost) << " 元" << std::endl;
    std::cout << "--------------------" << std::endl;

    char confirmChoice;
    std::cout << "确认支付并完成订单吗? (y/n): ";
    std::cin >> confirmChoice;
    clearInputBuffer();

    if (tolower(confirmChoice) != 'y') {
        std::cout << "订单已取消。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 提交直接购买请求
    std::string purchaseRequest = "DIRECT_PURCHASE|" + currentUsername + "|" + productName + "|" + sellerName + "|" + std::to_string(quantity);
    std::string purchaseResponse;

    std::cout << "正在处理订单，请稍候..." << std::endl;

    if (sendRequest(purchaseRequest, purchaseResponse)) {
        std::istringstream purchaseIss(purchaseResponse);
        std::string purchaseStatus, message, orderId;

        std::getline(purchaseIss, purchaseStatus, '|');
        std::getline(purchaseIss, message, '|');

        if (purchaseStatus == "PURCHASE_SUCCESS") {
            std::getline(purchaseIss, orderId, '|');
            std::cout << "购买成功! 订单ID: " << orderId << std::endl;
            std::cout << message << std::endl;
        } else if (purchaseStatus == "PURCHASE_PROCESSING") {
            std::getline(purchaseIss, orderId, '|');
            std::cout << "订单已提交，正在处理中... 订单ID: " << orderId << std::endl;
            std::cout << "请稍等，您可以稍后查询订单状态。" << std::endl;
        } else if (purchaseStatus == "PURCHASE_FAILED") {
            std::cout << "购买失败: " << message << std::endl;
        } else {
            std::cout << "未知错误，请联系客服。" << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

// 处理查看购物车功能
void ClientApp::handleViewCart() {
    if (currentUserType != "customer") {
        std::cout << "只有消费者才能查看购物车。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    std::string request = "VIEW_CART|" + currentUsername;
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, countStr;

        std::getline(iss, status, '|');

        if (status == "CART") {
            std::getline(iss, countStr, '|');
            int count = std::stoi(countStr);

            std::cout << "\n===== 购物车 =====\n";
            if (count == 0) {
                std::cout << "您的购物车是空的。" << std::endl;
            } else {
                std::cout << "购物车中共有 " << count << " 件商品:\n\n";
                std::cout << std::left << std::setw(5) << "序号"
                          << std::setw(20) << "商品名称"
                          << std::setw(15) << "商家"
                          << std::setw(10) << "单价"
                          << std::setw(10) << "数量"
                          << "小计" << std::endl;
                std::cout << std::string(80, '-') << std::endl;

                double totalAmount = 0.0;

                for (int i = 0; i < count; i++) {
                    std::string cartItem;
                    std::getline(iss, cartItem, '|');

                    std::istringstream itemStream(cartItem);
                    std::string name, seller, priceStr, quantityStr;

                    std::getline(itemStream, name, ',');
                    std::getline(itemStream, seller, ',');
                    std::getline(itemStream, priceStr, ',');
                    std::getline(itemStream, quantityStr, ',');

                    double price = std::stod(priceStr);
                    int quantity = std::stoi(quantityStr);
                    double subtotal = price * quantity;
                    totalAmount += subtotal;

                    std::cout << std::left << std::setw(5) << (i + 1)
                              << std::setw(20) << name
                              << std::setw(15) << seller
                              << std::setw(10) << std::fixed << std::setprecision(2) << price
                              << std::setw(10) << quantity
                              << std::fixed << std::setprecision(2) << subtotal << " 元" << std::endl;
                }

                std::cout << std::string(80, '-') << std::endl;
                std::cout << "总计: " << std::fixed << std::setprecision(2) << totalAmount << " 元" << std::endl;
            }
        } else {
            std::string errorMsg;
            std::getline(iss, errorMsg, '|');
            std::cout << "获取购物车失败: " << errorMsg << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "\n按回车键继续...";
    std::cin.get();
}

// 处理添加到购物车功能
void ClientApp::handleAddToCart() {
    if (currentUserType != "customer") {
        std::cout << "只有消费者才能添加商品到购物车。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    std::string productName, sellerName;
    int quantity;

    std::cout << "\n===== 添加商品到购物车 =====\n";
    std::cout << "请输入商品名称: ";
    std::getline(std::cin, productName);
    std::cout << "请输入商家名称: ";
    std::getline(std::cin, sellerName);
    std::cout << "请输入数量: ";
    std::cin >> quantity;
    clearInputBuffer();

    if (quantity <= 0) {
        std::cout << "数量必须大于0。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 先检查商品是否存在
    std::string detailRequest = "GET_PRODUCT_DETAIL|" + productName + "|" + sellerName;
    std::string detailResponse;

    if (!sendRequest(detailRequest, detailResponse)) {
        std::cout << "与服务器通信失败，无法获取商品详情。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    std::istringstream detailIss(detailResponse);
    std::string detailStatus;
    std::getline(detailIss, detailStatus, '|');

    if (detailStatus != "PRODUCT_DETAIL") {
        std::string errorMsg;
        std::getline(detailIss, errorMsg, '|');
        std::cout << "未找到商品: " << errorMsg << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 商品存在，获取库存
    std::string name, type, description, priceStr, quantityStr, seller;
    std::getline(detailIss, name, '|');
    std::getline(detailIss, type, '|');
    std::getline(detailIss, description, '|');
    std::getline(detailIss, priceStr, '|');
    std::getline(detailIss, quantityStr, '|');
    
    int stock = std::stoi(quantityStr);

    // 检查库存
    if (stock < quantity) {
        std::cout << "库存不足！当前库存: " << stock << std::endl;
        std::cout << "添加失败！按回车键继续...";
        std::cin.get();
        return;
    }

    // 添加到购物车
    std::string addRequest = "ADD_TO_CART|" + currentUsername + "|" + productName + "|" + sellerName + "|" + std::to_string(quantity);
    std::string addResponse;

    if (sendRequest(addRequest, addResponse)) {
        std::istringstream addIss(addResponse);
        std::string addStatus, message;

        std::getline(addIss, addStatus, '|');
        std::getline(addIss, message, '|');

        if (addStatus == "CART_UPDATED") {
            std::cout << "商品已成功添加到购物车！" << std::endl;
        } else {
            std::cout << "添加商品到购物车失败: " << message << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

// 处理结算购物车功能
void ClientApp::handleCheckout() {
    if (currentUserType != "customer") {
        std::cout << "只有消费者才能结算购物车。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 首先获取购物车内容
    std::string cartRequest = "VIEW_CART|" + currentUsername;
    std::string cartResponse;

    if (!sendRequest(cartRequest, cartResponse)) {
        std::cout << "与服务器通信失败，无法获取购物车内容。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    std::istringstream cartIss(cartResponse);
    std::string cartStatus, countStr;

    std::getline(cartIss, cartStatus, '|');
    std::getline(cartIss, countStr, '|');

    if (cartStatus != "CART" || std::stoi(countStr) == 0) {
        std::cout << "您的购物车是空的，无法结算。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 获取余额
    std::string balanceRequest = "CHECK_BALANCE|" + currentUsername;
    std::string balanceResponse;
    double balance = 0.0;

    if (sendRequest(balanceRequest, balanceResponse)) {
        std::istringstream balanceIss(balanceResponse);
        std::string balanceStatus, balanceStr;
        std::getline(balanceIss, balanceStatus, '|');

        if (balanceStatus == "BALANCE") {
            std::getline(balanceIss, balanceStr, '|');
            balance = std::stod(balanceStr);
        }
    }

    // 结算购物车
    std::string checkoutRequest = "CHECKOUT|" + currentUsername;
    std::string checkoutResponse;

    if (sendRequest(checkoutRequest, checkoutResponse)) {
        std::istringstream checkoutIss(checkoutResponse);
        std::string checkoutStatus, message, orderIdStr;

        std::getline(checkoutIss, checkoutStatus, '|');
        std::getline(checkoutIss, message, '|');

        if (checkoutStatus == "CHECKOUT_SUCCESS") {
            std::getline(checkoutIss, orderIdStr, '|');
            std::cout << "结算成功！订单ID: " << orderIdStr << std::endl;
            std::cout << message << std::endl;

            // 再次获取余额显示
            if (sendRequest(balanceRequest, balanceResponse)) {
                std::istringstream newBalanceIss(balanceResponse);
                std::string newBalanceStatus, newBalanceStr;
                std::getline(newBalanceIss, newBalanceStatus, '|');

                if (newBalanceStatus == "BALANCE") {
                    std::getline(newBalanceIss, newBalanceStr, '|');
                    double newBalance = std::stod(newBalanceStr);
                    std::cout << "您的当前余额: " << std::fixed << std::setprecision(2) << newBalance << " 元" << std::endl;
                }
            }
        } else if (checkoutStatus == "CHECKOUT_FAILED") {
            std::cout << "结算失败: " << message << std::endl;
        } else {
            std::cout << "未知错误，请联系客服。" << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

// 处理查看商家信息功能
void ClientApp::handleSellerInfo() {
    if (currentUserType != "seller") {
        std::cout << "此功能仅适用于商家用户。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    std::string request = "GET_SELLER_INFO|" + currentUsername;
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, username, incomeStr, productCountStr, creationTimeStr;

        std::getline(iss, status, '|');

        if (status == "SELLER_INFO") {
            std::getline(iss, username, '|');
            std::getline(iss, incomeStr, '|');
            std::getline(iss, productCountStr, '|');
            std::getline(iss, creationTimeStr, '|');

            double income = std::stod(incomeStr);
            int productCount = std::stoi(productCountStr);

            std::cout << "\n===== 商家信息 =====\n";
            std::cout << "商家名称: " << username << std::endl;
            std::cout << "累计收入: " << std::fixed << std::setprecision(2) << income << " 元" << std::endl;
            std::cout << "在售商品数: " << productCount << std::endl;
            std::cout << "账户创建时间: " << creationTimeStr << std::endl;
        } else {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "获取商家信息失败: " << message << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

// 处理商品管理功能
void ClientApp::handleManageProducts() {
    if (currentUserType != "seller") {
        std::cout << "只有商家才能管理商品。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    int choice;
    bool managing = true;

    while (managing) {
        std::cout << "\n===== 商品管理 =====\n";
        std::cout << "1. 查看我的商品\n";
        std::cout << "2. 添加新商品\n";
        std::cout << "3. 修改商品\n";
        std::cout << "4. 下架商品\n";
        std::cout << "0. 返回上一级\n";
        std::cout << "请选择: ";
        
        std::cin >> choice;
        clearInputBuffer();

        if (choice == 0) {
            managing = false;
            continue;
        }

        switch (choice) {
            case 1:
                // 查看我的商品
                // 根据client.cpp中的实现添加代码
                break;
            case 2:
                // 添加新商品
                // 根据client.cpp中的实现添加代码
                break;
            case 3:
                // 修改商品
                // 根据client.cpp中的实现添加代码
                break;
            case 4:
                // 下架商品
                // 根据client.cpp中的实现添加代码
                break;
            default:
                std::cout << "无效选择，请重试。" << std::endl;
                break;
        }
    }
}

// 处理查看收入功能
void ClientApp::handleCheckIncome() {
    if (currentUserType != "seller") {
        std::cout << "此功能仅适用于商家用户。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    std::string request = "CHECK_INCOME|" + currentUsername;
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, incomeStr;

        std::getline(iss, status, '|');

        if (status == "INCOME") {
            std::getline(iss, incomeStr, '|');
            double income = std::stod(incomeStr);

            std::cout << "\n===== 商家收入 =====\n";
            std::cout << "商家: " << currentUsername << std::endl;
            std::cout << "累计收入: " << std::fixed << std::setprecision(2) << income << " 元" << std::endl;
        } else {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "获取收入信息失败: " << message << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}
