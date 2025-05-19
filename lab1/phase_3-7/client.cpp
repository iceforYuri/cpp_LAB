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

// 全局变量
SOCKET clientSocket = INVALID_SOCKET;
std::string g_currentUsername;
std::string g_currentUserType;
bool g_isLoggedIn = false;

// 函数声明
bool connectToServer();
bool sendRequest(const std::string &request, std::string &response);
void clearInputBuffer();
int showMainMenu();
int showCustomerMenu();
int showSellerMenu();
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
void handleViewUserInfo();
void handleCheckBalance();
void handleDeposit();
void handleEnterStore();

int main()
{
    SetConsoleOutputCP(65001); // 设置控制台输出编码为 UTF-8
    SetConsoleCP(65001);       // 设置控制台输入编码为 UTF-8

    // 初始化 Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup 失败: " << result << std::endl;
        return 1;
    }

    // 连接到服务器
    if (!connectToServer())
    {
        std::cerr << "无法连接到服务器，请检查服务器是否运行" << std::endl;
        WSACleanup();
        return 1;
    }

    int menuResult = 1; // 1=继续, 0=退出程序, 2=退出登录, 3=进入商城
    while (menuResult != 0)
    {
        if (menuResult == -1)
        { // 处理退出登录
            handleLogout();
            menuResult = 1;
        }

        if (!g_isLoggedIn)
        {
            menuResult = showMainMenu();

            if (menuResult == 1)
            { // 登录
                handleLogin();
            }
            else if (menuResult == 2)
            { // 注册
                handleRegister();
            }
        }
        else
        { // 已登录
            if (g_currentUserType == "customer")
            {
                menuResult = showCustomerMenu();

                if (menuResult == 1)
                { // 查看个人信息
                    handleViewUserInfo();
                }
                else if (menuResult == 2)
                { // 修改密码
                    handleChangePassword();
                }
                else if (menuResult == 3)
                { // 查询余额
                    handleCheckBalance();
                }
                else if (menuResult == 4)
                { // 充值
                    handleDeposit();
                }
                else if (menuResult == 5)
                { // 进入商城
                    handleEnterStore();
                }
            }
            else if (g_currentUserType == "seller")
            {
                menuResult = showSellerMenu();

                if (menuResult == 1)
                { // 查看商户信息
                    handleSellerInfo();
                }
                else if (menuResult == 2)
                { // 修改密码
                    handleChangePassword();
                }
                else if (menuResult == 3)
                { // 进入商城(管理)
                    handleManageProducts();
                }
                else if (menuResult == 4)
                { // 查看收入
                    handleCheckIncome();
                }
            }
        }
    }
    handleLogout();
    // 断开连接并清理
    if (clientSocket != INVALID_SOCKET)
    {
        closesocket(clientSocket);
    }
    WSACleanup();

    return 0;
}

bool connectToServer()
{
    // 创建客户端socket
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)
    {
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
    if (connect(clientSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "连接失败: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        return false;
    }

    std::cout << "已连接到服务器。" << std::endl;
    return true;
}

bool sendRequest(const std::string &request, std::string &response)
{
    // 发送请求
    if (send(clientSocket, request.c_str(), request.size(), 0) == SOCKET_ERROR)
    {
        std::cerr << "发送失败: " << WSAGetLastError() << std::endl;
        return false;
    }

    // 接收响应
    char buffer[4096] = {0};
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0)
    {
        std::cerr << "接收失败或连接关闭: " << WSAGetLastError() << std::endl;
        return false;
    }

    response = std::string(buffer, bytesReceived);
    return true;
}

void clearInputBuffer()
{
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int showMainMenu()
{
    int choice;

    std::cout << "\n===== 欢迎使用网络电子商城系统 =====\n";
    std::cout << "1. 登录\n";
    std::cout << "2. 注册\n";
    std::cout << "0. 退出\n";
    std::cout << "请选择: ";

    std::cin >> choice;
    clearInputBuffer();

    if (choice < 0 || choice > 2)
    {
        std::cout << "无效选择，请重试。\n";
        return 1; // 默认返回继续
    }

    return choice;
}

int showCustomerMenu()
{
    int choice;

    std::cout << "\n\n\n--- 消费者菜单 (" << g_currentUsername << ") ---\n";
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
    if (choice < 0 || choice > 6)
    {
        std::cout << "无效选择，请重试。\n";
        return 1; // 默认返回继续
    }

    return choice;
}

int showSellerMenu()
{
    int choice;

    std::cout << "\n===== 商家菜单 (" << g_currentUsername << ") =====\n";
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
    if (choice < 0 || choice > 5)
    {
        std::cout << "无效选择，请重试。\n";
        return 1; // 默认返回继续
    }

    return choice;
}

void handleLogin()
{
    std::string username, password;

    std::cout << "\n===== 用户登录 =====\n";
    std::cout << "用户名: ";
    std::getline(std::cin, username);
    std::cout << "密码: ";
    std::getline(std::cin, password);

    std::string request = "LOGIN|" + username + "|" + password;
    std::string response;

    if (sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, message, userType;

        std::getline(iss, status, '|');

        if (status == "LOGIN_SUCCESS")
        {
            std::getline(iss, message, '|');
            std::getline(iss, userType, '|');

            g_isLoggedIn = true;
            g_currentUsername = message;
            g_currentUserType = userType;

            std::cout << "登录成功！欢迎 " << g_currentUsername << "(" << g_currentUserType << ")" << std::endl;
        }
        else
        {
            std::getline(iss, message, '|');
            std::cout << "登录失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }
}

void handleRegister()
{
    std::string username, password, confirmPassword, typeChoice;
    int choice;

    std::cout << "\n===== 用户注册 =====\n";
    std::cout << "用户名: ";
    std::getline(std::cin, username);
    std::cout << "密码: ";
    std::getline(std::cin, password);
    std::cout << "确认密码: ";
    std::getline(std::cin, confirmPassword);

    if (password != confirmPassword)
    {
        std::cout << "两次输入的密码不一致，请重试。" << std::endl;
        return;
    }

    std::cout << "账户类型 (1: 消费者, 2: 商家): ";
    std::cin >> choice;
    clearInputBuffer();

    if (choice == 1)
    {
        typeChoice = "customer";
    }
    else if (choice == 2)
    {
        typeChoice = "seller";
    }
    else
    {
        std::cout << "无效选择，请重试。" << std::endl;
        return;
    }

    std::string request = "REGISTER|" + username + "|" + password + "|" + typeChoice;
    std::string response;

    if (sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, message;

        std::getline(iss, status, '|');
        std::getline(iss, message, '|');

        if (status == "REGISTER_SUCCESS")
        {
            std::cout << "注册成功！用户名: " << message << std::endl;
        }
        else
        {
            std::cout << "注册失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }
}

void handleBrowseProducts()
{
    std::string request = "GET_PRODUCTS";
    std::string response;

    if (sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, countStr;

        std::getline(iss, status, '|');

        if (status == "PRODUCTS")
        {
            std::getline(iss, countStr, '|');
            int count = std::stoi(countStr);

            std::cout << "\n===== 商品列表 =====\n";
            std::cout << "共 " << count << " 个商品\n\n";

            for (int i = 0; i < count; i++)
            {
                std::string item;
                std::getline(iss, item, '|');

                std::istringstream itemStream(item);
                std::string name, type, priceStr, quantityStr, seller;

                std::getline(itemStream, name, ',');
                std::getline(itemStream, type, ',');
                std::getline(itemStream, priceStr, ',');
                std::getline(itemStream, quantityStr, ',');
                std::getline(itemStream, seller, ',');

                std::cout << i + 1 << ". " << name << " (" << type << ")\n";
                std::cout << "   价格: " << priceStr << " 元, 库存: " << quantityStr << ", 商家: " << seller << "\n\n";
            }
        }
        else
        {
            std::cout << "获取商品列表失败。" << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void handleProductDetail()
{
    std::string productName, sellerName;

    std::cout << "\n===== 商品详情 =====\n";
    std::cout << "输入商品名称: ";
    std::getline(std::cin, productName);
    std::cout << "输入商家名称: ";
    std::getline(std::cin, sellerName);

    std::string request = "GET_PRODUCT_DETAIL|" + productName + "|" + sellerName;
    std::string response;

    if (sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status;

        std::getline(iss, status, '|');

        if (status == "PRODUCT_DETAIL")
        {
            std::string name, type, description, priceStr, quantityStr, seller;

            std::getline(iss, name, '|');
            std::getline(iss, type, '|');
            std::getline(iss, description, '|');
            std::getline(iss, priceStr, '|');
            std::getline(iss, quantityStr, '|');
            std::getline(iss, seller, '|');

            std::cout << "商品名称: " << name << "\n";
            std::cout << "类型: " << type << "\n";
            std::cout << "描述: " << description << "\n";
            std::cout << "价格: " << priceStr << " 元\n";
            std::cout << "库存: " << quantityStr << "\n";
            std::cout << "商家: " << seller << "\n";

            if (type == "Book")
            {
                std::string author, isbn;
                std::getline(iss, author, '|');
                std::getline(iss, isbn, '|');
                std::cout << "作者: " << author << "\n";
                std::cout << "ISBN: " << isbn << "\n";
            }
            else if (type == "Clothing")
            {
                std::string size, color;
                std::getline(iss, size, '|');
                std::getline(iss, color, '|');
                std::cout << "尺寸: " << size << "\n";
                std::cout << "颜色: " << color << "\n";
            }
            else if (type == "Food")
            {
                std::string expDate;
                std::getline(iss, expDate, '|');
                std::cout << "保质期: " << expDate << "\n";
            }
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "获取商品详情失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void handleViewCart()
{
    if (!g_isLoggedIn || g_currentUserType != "customer")
    {
        std::cout << "请先以消费者身份登录。" << std::endl;
        return;
    }

    std::string request = "GET_CART|" + g_currentUsername;
    std::string response;

    if (sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, countStr;

        std::getline(iss, status, '|');

        if (status == "CART")
        {
            std::getline(iss, countStr, '|');
            int count = std::stoi(countStr);

            if (count == 0)
            {
                std::cout << "\n您的购物车是空的。" << std::endl;
                std::cout << "按回车键继续...";
                std::cin.get();
                return;
            }

            // 解析购物车项目
            std::vector<std::pair<std::string, std::string>> cartItems; // 商品名称，商家名称
            std::vector<int> quantities;                                // 数量
            std::vector<double> prices;                                 // 单价
            double totalAmount = 0.0;

            std::cout << "\n--- 我的购物车 ---" << std::endl;
            std::cout << std::fixed << std::setprecision(2); // 设置输出格式

            for (int i = 0; i < count; i++)
            {
                std::string item;
                std::getline(iss, item, '|');

                std::istringstream itemStream(item);
                std::string name, quantityStr, priceStr, seller;

                std::getline(itemStream, name, ',');
                std::getline(itemStream, quantityStr, ',');
                std::getline(itemStream, priceStr, ',');
                std::getline(itemStream, seller, ',');

                int quantity = std::stoi(quantityStr);
                double price = std::stod(priceStr);
                double itemTotal = quantity * price;
                totalAmount += itemTotal;

                cartItems.push_back(std::make_pair(name, seller));
                quantities.push_back(quantity);
                prices.push_back(price);

                std::cout << i + 1 << ". 商品: " << name
                          << " | 数量: " << quantity
                          << " | 当前单价: ¥" << price
                          << " | 小计: ¥" << itemTotal << std::endl;

                // 获取商品详情，检查库存和价格变化
                std::string detailRequest = "GET_PRODUCT_DETAIL|" + name + "|" + seller;
                std::string detailResponse;

                if (sendRequest(detailRequest, detailResponse))
                {
                    std::istringstream detailIss(detailResponse);
                    std::string detailStatus;
                    std::getline(detailIss, detailStatus, '|');

                    if (detailStatus == "PRODUCT_DETAIL")
                    {
                        std::string productName, productType, productDesc, currentPriceStr, currentQuantityStr, productSeller;
                        std::getline(detailIss, productName, '|');
                        std::getline(detailIss, productType, '|');
                        std::getline(detailIss, productDesc, '|');
                        std::getline(detailIss, currentPriceStr, '|');
                        std::getline(detailIss, currentQuantityStr, '|');

                        double currentPrice = std::stod(currentPriceStr);
                        int currentQuantity = std::stoi(currentQuantityStr);

                        if (currentQuantity < quantity)
                        {
                            std::cout << "   注意: " << name << " 当前库存(" << currentQuantity
                                      << ")不足购物车数量(" << quantity << ")!" << std::endl;
                        }

                        if (currentPrice != price)
                        {
                            std::cout << "   提示: " << name << " 加入时价格为 ¥" << price
                                      << ", 当前价格已变为 ¥" << currentPrice << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "   警告: 商品 " << name << " 可能已从商店下架!" << std::endl;
                    }
                }
            }

            std::cout << "--------------------" << std::endl;
            std::cout << "购物车总计: ¥" << totalAmount << std::endl;
            std::cout << "--------------------" << std::endl;

            // 购物车操作菜单
            int cartChoice = -1;
            do
            {
                std::cout << "\n购物车操作:" << std::endl;
                std::cout << "1. 修改商品数量" << std::endl;
                std::cout << "2. 移除商品" << std::endl;
                std::cout << "3. 生成订单并结算" << std::endl;
                std::cout << "0. 返回商城" << std::endl;
                std::cout << "请选择: ";

                if (!(std::cin >> cartChoice))
                {
                    std::cout << "无效输入。" << std::endl;
                    clearInputBuffer();
                    cartChoice = -1;
                    continue;
                }
                clearInputBuffer();

                switch (cartChoice)
                {
                case 1: // 修改商品数量
                {
                    std::string nameToUpdate;
                    std::string sellerName;
                    int newQty;

                    std::cout << "输入要修改数量的商品名称: ";
                    std::getline(std::cin >> std::ws, nameToUpdate);

                    // 查找商品对应的商家
                    bool found = false;
                    for (size_t i = 0; i < cartItems.size(); i++)
                    {
                        if (cartItems[i].first == nameToUpdate)
                        {
                            sellerName = cartItems[i].second;
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        std::cout << "未在购物车中找到商品: " << nameToUpdate << std::endl;
                        break;
                    }

                    std::cout << "输入新的数量 (输入0将移除该商品): ";
                    if (!(std::cin >> newQty) || newQty < 0)
                    {
                        std::cout << "无效数量输入。" << std::endl;
                        clearInputBuffer();
                        break;
                    }
                    clearInputBuffer();

                    // 发送更新请求
                    std::string updateRequest;
                    if (newQty == 0)
                    {
                        updateRequest = "REMOVE_FROM_CART|" + g_currentUsername + "|" + nameToUpdate + "|" + sellerName;
                    }
                    else
                    {
                        updateRequest = "UPDATE_CART_ITEM|" + g_currentUsername + "|" + nameToUpdate + "|" + sellerName + "|" + std::to_string(newQty);
                    }

                    std::string updateResponse;
                    if (sendRequest(updateRequest, updateResponse))
                    {
                        std::istringstream updateIss(updateResponse);
                        std::string updateStatus, updateMessage;

                        std::getline(updateIss, updateStatus, '|');
                        std::getline(updateIss, updateMessage, '|');

                        if (updateStatus == "CART_UPDATED")
                        {
                            std::cout << "购物车已更新: " << updateMessage << std::endl;
                            cartChoice = 0; // 刷新购物车
                            handleViewCart();
                        }
                        else
                        {
                            std::cout << "更新失败: " << updateMessage << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "与服务器通信失败。" << std::endl;
                    }
                    break;
                }
                case 2: // 移除商品
                {
                    std::string nameToRemove;
                    std::string sellerName;

                    std::cout << "输入要移除的商品名称: ";
                    std::getline(std::cin >> std::ws, nameToRemove);

                    // 查找商品对应的商家
                    bool found = false;
                    for (size_t i = 0; i < cartItems.size(); i++)
                    {
                        if (cartItems[i].first == nameToRemove)
                        {
                            sellerName = cartItems[i].second;
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        std::cout << "未在购物车中找到商品: " << nameToRemove << std::endl;
                        break;
                    }

                    // 发送移除请求
                    std::string removeRequest = "REMOVE_FROM_CART|" + g_currentUsername + "|" + nameToRemove + "|" + sellerName;
                    std::string removeResponse;

                    if (sendRequest(removeRequest, removeResponse))
                    {
                        std::istringstream removeIss(removeResponse);
                        std::string removeStatus, removeMessage;

                        std::getline(removeIss, removeStatus, '|');
                        std::getline(removeIss, removeMessage, '|');

                        if (removeStatus == "CART_UPDATED")
                        {
                            std::cout << "购物车已更新: " << removeMessage << std::endl;
                            cartChoice = 0; // 刷新购物车
                            handleViewCart();
                        }
                        else
                        {
                            std::cout << "移除失败: " << removeMessage << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "与服务器通信失败。" << std::endl;
                    }
                    break;
                }
                case 3: // 生成订单并结算
                {
                    // 首先获取用户余额
                    std::string balanceRequest = "CHECK_BALANCE|" + g_currentUsername;
                    std::string balanceResponse;
                    double balance = 0.0;

                    if (sendRequest(balanceRequest, balanceResponse))
                    {
                        std::istringstream balanceIss(balanceResponse);
                        std::string balanceStatus, balanceStr;

                        std::getline(balanceIss, balanceStatus, '|');
                        if (balanceStatus == "BALANCE")
                        {
                            std::getline(balanceIss, balanceStr, '|');
                            balance = std::stod(balanceStr);
                        }
                    }

                    // 显示订单预览
                    std::cout << "\n===== 订单预览 =====\n";
                    for (size_t i = 0; i < cartItems.size(); i++)
                    {
                        std::cout << i + 1 << ". " << cartItems[i].first
                                  << " x " << quantities[i]
                                  << " @ ¥" << prices[i]
                                  << " = ¥" << (quantities[i] * prices[i]) << std::endl;
                    }
                    std::cout << "--------------------" << std::endl;
                    std::cout << "订单总计: ¥" << totalAmount << std::endl;
                    std::cout << "您的当前余额: ¥" << std::fixed << std::setprecision(2) << balance << std::endl;
                    std::cout << "支付后余额将为: ¥" << (balance - totalAmount) << std::endl;
                    std::cout << "--------------------" << std::endl;

                    if (balance < totalAmount)
                    {
                        std::cout << "错误: 您的余额不足以支付此订单！" << std::endl;
                        break;
                    }

                    // 确认支付
                    char confirmChoice;
                    std::cout << "确认支付并完成订单吗? (y/n): ";
                    std::cin >> confirmChoice;
                    clearInputBuffer();

                    if (tolower(confirmChoice) != 'y')
                    {
                        std::cout << "订单已取消。" << std::endl;
                        break;
                    }

                    // 提交结算请求
                    std::cout << "正在处理订单，请稍候..." << std::endl;
                    std::string checkoutRequest = "CHECKOUT|" + g_currentUsername;
                    std::string checkoutResponse;

                    if (sendRequest(checkoutRequest, checkoutResponse))
                    {
                        std::istringstream checkoutIss(checkoutResponse);
                        std::string checkoutStatus, checkoutMessage, orderId;

                        std::getline(checkoutIss, checkoutStatus, '|');
                        std::getline(checkoutIss, checkoutMessage, '|');
                        if (checkoutStatus == "CHECKOUT_SUCCESS")
                        {
                            std::getline(checkoutIss, orderId, '|');
                            std::cout << "结算成功: " << checkoutMessage << std::endl;
                            std::cout << "订单ID: " << orderId << std::endl;

                            // 订单处理成功，返回商城
                            cartChoice = 0;
                        }
                        else if (checkoutStatus == "CHECKOUT_PENDING")
                        {
                            std::getline(checkoutIss, orderId, '|');
                            std::cout << "订单已提交但正在处理中: " << checkoutMessage << std::endl;
                            std::cout << "订单ID: " << orderId << std::endl;
                            std::cout << "您可以稍后在订单记录中查看订单状态。" << std::endl;

                            // 返回商城
                            cartChoice = 0;
                        }
                        else
                        {
                            std::cout << "结算失败: " << checkoutMessage << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "与服务器通信失败。" << std::endl;
                    }
                    break;
                }
                case 0: // 返回商城
                    break;
                case -2:                     // 刷新购物车
                    return handleViewCart(); // 递归调用以刷新购物车
                default:
                    std::cout << "无效选项。" << std::endl;
                }

            } while (cartChoice != 0);
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "获取购物车失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    // std::cout << "按回车键继续...";
    // std::cin.get();
}

void handleAddToCart()
{
    if (!g_isLoggedIn || g_currentUserType != "customer")
    {
        std::cout << "请先以消费者身份登录。" << std::endl;
        return;
    }

    std::string productName, sellerName;
    int quantity;

    std::cout << "\n===== 添加商品到购物车 =====\n";
    std::cout << "输入商品名称: ";
    std::getline(std::cin, productName);
    std::cout << "输入商家名称: ";
    std::getline(std::cin, sellerName);
    std::cout << "输入数量: ";
    std::cin >> quantity;
    clearInputBuffer();

    if (quantity <= 0)
    {
        std::cout << "数量必须大于0。" << std::endl;
        return;
    }

    std::string request = "ADD_TO_CART|" + g_currentUsername + "|" + productName + "|" + sellerName + "|" + std::to_string(quantity);
    std::string response;

    if (sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, message;

        std::getline(iss, status, '|');
        std::getline(iss, message, '|');

        if (status == "CART_UPDATED")
        {
            std::cout << "成功: " << message << std::endl;
        }
        else
        {
            std::cout << "添加失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void handleCheckout()
{
    if (!g_isLoggedIn || g_currentUserType != "customer")
    {
        std::cout << "请先以消费者身份登录。" << std::endl;
        return;
    }

    std::string request = "CHECKOUT|" + g_currentUsername;
    std::string response;

    if (sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, message, orderId;

        std::getline(iss, status, '|');
        std::getline(iss, message, '|');

        if (status == "CHECKOUT_SUCCESS")
        {
            std::getline(iss, orderId, '|');
            std::cout << "结算成功: " << message << "\n";
            std::cout << "订单号: " << orderId << std::endl;
        }
        else
        {
            std::cout << "结算失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

// 商家功能实现

void handleSellerInfo()
{
    if (!g_isLoggedIn || g_currentUserType != "seller")
    {
        std::cout << "请先以商家身份登录。" << std::endl;
        return;
    }

    std::string request = "GET_SELLER_INFO|" + g_currentUsername;
    std::string response;

    if (sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status;

        std::getline(iss, status, '|');

        if (status == "SELLER_INFO")
        {
            std::string username, userType, balanceStr;

            std::getline(iss, username, '|');
            std::getline(iss, userType, '|');
            std::getline(iss, balanceStr, '|');

            std::cout << "\n===== 商家信息 =====\n";
            std::cout << "用户名: " << username << "\n";
            std::cout << "账户类型: " << userType << "\n";
            std::cout << "账户余额: " << balanceStr << " 元\n";
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "获取商家信息失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void handleChangePassword()
{
    if (!g_isLoggedIn)
    {
        std::cout << "请先登录。" << std::endl;
        return;
    }

    std::string oldPassword, newPassword;

    std::cout << "\n===== 修改密码 =====\n";
    std::cout << "请输入原密码: ";
    std::getline(std::cin, oldPassword);
    std::cout << "请输入新密码: ";
    std::getline(std::cin, newPassword);

    std::string request = "CHANGE_PASSWORD|" + g_currentUsername + "|" + oldPassword + "|" + newPassword;
    std::string response;

    if (sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, message;

        std::getline(iss, status, '|');
        std::getline(iss, message, '|');

        if (status == "PASSWORD_CHANGED")
        {
            std::cout << "密码修改成功。" << std::endl;
        }
        else
        {
            std::cout << "密码修改失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void handleManageProducts()
{
    if (!g_isLoggedIn || g_currentUserType != "seller")
    {
        std::cout << "请先以商家身份登录。" << std::endl;
        return;
    }

    while (true)
    {
        int choice;

        std::cout << "\n===== 商品管理 =====\n";
        std::cout << "1. 查看我的商品\n";
        std::cout << "2. 添加新商品\n";
        std::cout << "3. 修改商品价格\n";
        std::cout << "4. 修改商品库存\n";
        std::cout << "5. 设置商品折扣\n";
        std::cout << "0. 返回上级菜单\n";
        std::cout << "请选择: ";

        std::cin >> choice;
        clearInputBuffer();

        if (choice == 0)
        {
            break;
        }

        switch (choice)
        {
        case 1:
        {
            // 查看我的商品
            std::string request = "GET_SELLER_PRODUCTS|" + g_currentUsername;
            std::string response;

            if (sendRequest(request, response))
            {
                std::istringstream iss(response);
                std::string status, countStr;

                std::getline(iss, status, '|');

                if (status == "SELLER_PRODUCTS")
                {
                    std::getline(iss, countStr, '|');
                    int count = std::stoi(countStr);

                    std::cout << "\n===== 我的商品 =====\n";
                    std::cout << "共 " << count << " 个商品\n\n";

                    for (int i = 0; i < count; i++)
                    {
                        std::string item;
                        std::getline(iss, item, '|');

                        std::istringstream itemStream(item);
                        std::string name, type, priceStr, quantityStr, discountStr;

                        std::getline(itemStream, name, ',');
                        std::getline(itemStream, type, ',');
                        std::getline(itemStream, priceStr, ',');
                        std::getline(itemStream, quantityStr, ',');
                        std::getline(itemStream, discountStr, ',');

                        double discount = std::stod(discountStr);
                        double displayPrice = std::stod(priceStr) * (1.0 - discount);

                        std::cout << i + 1 << ". " << name << " (" << type << ")\n";
                        std::cout << "   原价: " << priceStr << " 元, 折扣: " << discount * 100 << "%\n";
                        std::cout << "   售价: " << displayPrice << " 元, 库存: " << quantityStr << "\n\n";
                    }
                }
                else
                {
                    std::string message;
                    std::getline(iss, message, '|');
                    std::cout << "获取商品列表失败: " << message << std::endl;
                }
            }
            else
            {
                std::cout << "与服务器通信失败。" << std::endl;
            }

            std::cout << "按回车键继续...";
            std::cin.get();
            break;
        }
        case 2:
        {
            // 添加新商品
            int productType;
            std::string name, description;
            double price;
            int quantity;

            std::cout << "\n===== 添加新商品 =====\n";
            std::cout << "选择商品类型 (1: 书籍, 2: 服装, 3: 食品, 4: 其他): ";
            std::cin >> productType;
            clearInputBuffer();

            std::cout << "商品名称: ";
            std::getline(std::cin, name);
            std::cout << "商品描述: ";
            std::getline(std::cin, description);
            std::cout << "商品价格: ";
            std::cin >> price;
            std::cout << "商品库存: ";
            std::cin >> quantity;
            clearInputBuffer();

            if (price < 0 || quantity < 0)
            {
                std::cout << "价格和库存不能为负数。" << std::endl;
                break;
            }

            std::string request;

            if (productType == 1)
            { // 书籍
                std::string author, isbn;
                std::cout << "作者: ";
                std::getline(std::cin, author);
                std::cout << "ISBN: ";
                std::getline(std::cin, isbn);

                request = "ADD_BOOK|" + g_currentUsername + "|" + name + "|" + description + "|" +
                          std::to_string(price) + "|" + std::to_string(quantity) + "|" + author + "|" + isbn;
            }
            else if (productType == 2)
            { // 服装
                std::string size, color;
                std::cout << "尺寸: ";
                std::getline(std::cin, size);
                std::cout << "颜色: ";
                std::getline(std::cin, color);

                request = "ADD_CLOTHING|" + g_currentUsername + "|" + name + "|" + description + "|" +
                          std::to_string(price) + "|" + std::to_string(quantity) + "|" + size + "|" + color;
            }
            else if (productType == 3)
            { // 食品
                std::string expDate;
                std::cout << "保质期 (YYYY-MM-DD): ";
                std::getline(std::cin, expDate);

                request = "ADD_FOOD|" + g_currentUsername + "|" + name + "|" + description + "|" +
                          std::to_string(price) + "|" + std::to_string(quantity) + "|" + expDate;
            }
            else if (productType == 4)
            { // 其他
                std::string category;
                std::cout << "商品分类: ";
                std::getline(std::cin, category);

                request = "ADD_GENERIC|" + g_currentUsername + "|" + name + "|" + description + "|" +
                          std::to_string(price) + "|" + std::to_string(quantity) + "|" + category;
            }
            else
            {
                std::cout << "无效的商品类型。" << std::endl;
                break;
            }

            std::string response;

            if (sendRequest(request, response))
            {
                std::istringstream iss(response);
                std::string status, message;

                std::getline(iss, status, '|');
                std::getline(iss, message, '|');

                if (status == "PRODUCT_ADDED")
                {
                    std::cout << "添加成功: " << message << std::endl;
                }
                else
                {
                    std::cout << "添加失败: " << message << std::endl;
                }
            }
            else
            {
                std::cout << "与服务器通信失败。" << std::endl;
            }

            std::cout << "按回车键继续...";
            std::cin.get();
            break;
        }
        case 3:
        {
            // 修改商品价格
            std::string productName;
            double newPrice;

            std::cout << "\n===== 修改商品价格 =====\n";
            std::cout << "商品名称: ";
            std::getline(std::cin, productName);
            std::cout << "新价格: ";
            std::cin >> newPrice;
            clearInputBuffer();

            if (newPrice < 0)
            {
                std::cout << "价格不能为负数。" << std::endl;
                break;
            }

            std::string request = "UPDATE_PRODUCT_PRICE|" + g_currentUsername + "|" + productName + "|" + std::to_string(newPrice);
            std::string response;

            if (sendRequest(request, response))
            {
                std::istringstream iss(response);
                std::string status, message;

                std::getline(iss, status, '|');
                std::getline(iss, message, '|');

                if (status == "PRODUCT_UPDATED")
                {
                    std::cout << "价格修改成功: " << message << std::endl;
                }
                else
                {
                    std::cout << "价格修改失败: " << message << std::endl;
                }
            }
            else
            {
                std::cout << "与服务器通信失败。" << std::endl;
            }

            std::cout << "按回车键继续...";
            std::cin.get();
            break;
        }
        case 4:
        {
            // 修改商品库存
            std::string productName;
            int newQuantity;

            std::cout << "\n===== 修改商品库存 =====\n";
            std::cout << "商品名称: ";
            std::getline(std::cin, productName);
            std::cout << "新库存: ";
            std::cin >> newQuantity;
            clearInputBuffer();

            if (newQuantity < 0)
            {
                std::cout << "库存不能为负数。" << std::endl;
                break;
            }

            std::string request = "UPDATE_PRODUCT_QUANTITY|" + g_currentUsername + "|" + productName + "|" + std::to_string(newQuantity);
            std::string response;

            if (sendRequest(request, response))
            {
                std::istringstream iss(response);
                std::string status, message;

                std::getline(iss, status, '|');
                std::getline(iss, message, '|');

                if (status == "PRODUCT_UPDATED")
                {
                    std::cout << "库存修改成功: " << message << std::endl;
                }
                else
                {
                    std::cout << "库存修改失败: " << message << std::endl;
                }
            }
            else
            {
                std::cout << "与服务器通信失败。" << std::endl;
            }

            std::cout << "按回车键继续...";
            std::cin.get();
            break;
        }
        case 5:
        {
            // 设置商品折扣
            int discountType;

            std::cout << "\n===== 设置商品折扣 =====\n";
            std::cout << "选择折扣类型 (1: 单个商品, 2: 商品分类): ";
            std::cin >> discountType;
            clearInputBuffer();

            if (discountType == 1)
            {
                std::string productName;
                double discount;

                std::cout << "商品名称: ";
                std::getline(std::cin, productName);
                std::cout << "折扣率 (0-100): ";
                std::cin >> discount;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                if (discount < 0 || discount > 100)
                {
                    std::cout << "折扣率必须在0-100之间。" << std::endl;
                    break;
                }

                std::string request = "SET_PRODUCT_DISCOUNT|" + g_currentUsername + "|" + productName + "|" + std::to_string(discount / 100);
                std::string response;

                if (sendRequest(request, response))
                {
                    std::istringstream iss(response);
                    std::string status, message;

                    std::getline(iss, status, '|');
                    std::getline(iss, message, '|');

                    if (status == "PRODUCT_UPDATED")
                    {
                        std::cout << "折扣设置成功: " << message << std::endl;
                    }
                    else
                    {
                        std::cout << "折扣设置失败: " << message << std::endl;
                    }
                }
                else
                {
                    std::cout << "与服务器通信失败。" << std::endl;
                }
            }
            else if (discountType == 2)
            {
                std::string category;
                double discount;

                std::cout << "商品分类: ";
                std::getline(std::cin, category);
                std::cout << "折扣率 (0.0-1.0): ";
                std::cin >> discount;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                if (discount < 0.0 || discount > 1.0)
                {
                    std::cout << "折扣率必须在0.0-1.0之间。" << std::endl;
                    break;
                }

                std::string request = "SET_CATEGORY_DISCOUNT|" + g_currentUsername + "|" + category + "|" + std::to_string(discount);
                std::string response;

                if (sendRequest(request, response))
                {
                    std::istringstream iss(response);
                    std::string status, message;

                    std::getline(iss, status, '|');
                    std::getline(iss, message, '|');

                    if (status == "CATEGORY_UPDATED")
                    {
                        std::cout << "分类折扣设置成功: " << message << std::endl;
                    }
                    else
                    {
                        std::cout << "分类折扣设置失败: " << message << std::endl;
                    }
                }
                else
                {
                    std::cout << "与服务器通信失败。" << std::endl;
                }
            }
            else
            {
                std::cout << "无效的折扣类型。" << std::endl;
            }

            std::cout << "按回车键继续...";
            std::cin.get();
            break;
        }
        default:
            std::cout << "无效选择，请重试。" << std::endl;
            break;
        }
    }
}

void handleCheckIncome()
{
    if (!g_isLoggedIn || g_currentUserType != "seller")
    {
        std::cout << "请先以商家身份登录。" << std::endl;
        return;
    }

    std::string request = "CHECK_BALANCE|" + g_currentUsername;
    std::string response;

    if (sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, balanceStr;

        std::getline(iss, status, '|');

        if (status == "BALANCE")
        {
            std::getline(iss, balanceStr, '|');
            double balance = std::stod(balanceStr);

            std::cout << "\n===== 账户收入 =====\n";
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "当前收入: " << balance << " 元\n";
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "查询收入失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void handleViewUserInfo()
{
    if (!g_isLoggedIn)
    {
        std::cout << "请先登录。" << std::endl;
        return;
    }

    std::string request = "GET_USER_INFO|" + g_currentUsername;
    std::string response;

    if (sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status;

        std::getline(iss, status, '|');

        if (status == "USER_INFO")
        {
            std::string username, userType, balanceStr;

            std::getline(iss, username, '|');
            std::getline(iss, userType, '|');
            std::getline(iss, balanceStr, '|');

            std::cout << "\n===== 用户信息 =====\n";
            std::cout << "用户名: " << username << "\n";
            std::cout << "账户类型: " << userType << "\n";
            std::cout << "账户余额: " << balanceStr << " 元\n";
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "获取用户信息失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void handleCheckBalance()
{
    if (!g_isLoggedIn)
    {
        std::cout << "请先登录。" << std::endl;
        return;
    }

    std::string request = "CHECK_BALANCE|" + g_currentUsername;
    std::string response;

    if (sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, balanceStr;

        std::getline(iss, status, '|');

        if (status == "BALANCE")
        {
            std::getline(iss, balanceStr, '|');
            double balance = std::stod(balanceStr);

            std::cout << "\n===== 账户余额 =====\n";
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "当前余额: " << balance << " 元\n";
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "查询余额失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void handleDeposit()
{
    if (!g_isLoggedIn)
    {
        std::cout << "请先登录。" << std::endl;
        return;
    }

    double amount;
    std::cout << "\n===== 账户充值 =====\n";
    std::cout << "请输入充值金额: ";
    if (!(std::cin >> amount) || amount <= 0)
    {
        std::cout << "无效金额。" << std::endl;
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    std::string request = "DEPOSIT|" + g_currentUsername + "|" + std::to_string(amount);
    std::string response;

    if (sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, message, balanceStr;

        std::getline(iss, status, '|');

        if (status == "DEPOSIT_SUCCESS")
        {
            std::getline(iss, message, '|');
            std::getline(iss, balanceStr, '|');
            double balance = std::stod(balanceStr);

            std::cout << "充值成功。当前余额: " << std::fixed << std::setprecision(2) << balance << " 元" << std::endl;
        }
        else
        {
            std::getline(iss, message, '|');
            std::cout << "充值失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void handleEnterStore()
{
    int choice = -1;

    do
    {
        std::cout << "\n--- 商城 (用户: " << g_currentUsername << ") ---\n";
        std::cout << "1. 显示所有商品\n";
        std::cout << "2. 按名称搜索商品\n";
        std::cout << "3. 直接购买商品\n";
        std::cout << "4. 加入购物车\n";
        std::cout << "5. 查看我的购物车\n";
        std::cout << "0. 退出商城\n";
        std::cout << "请选择: ";

        if (!(std::cin >> choice))
        {
            std::cout << "无效输入。" << std::endl;
            clearInputBuffer();
            choice = -1;
            continue;
        }
        clearInputBuffer();

        switch (choice)
        {
        case 1:
            handleBrowseProducts();
            break;
        case 2:
            handleSearchProducts();
            break;
        case 3:
            handleDirectPurchase();
            break;
        case 4:
            handleAddToCart();
            break;
        case 5:
            handleViewCart();
            break;
        case 0:
            std::cout << "正在退出商城..." << std::endl;
            break;
        default:
            std::cout << "无效选项。" << std::endl;
        }
    } while (choice != 0);
}

void handleLogout()
{
    if (!g_isLoggedIn)
    {
        std::cout << "您当前未登录！" << std::endl;
        return;
    }

    std::string request = "LOGOUT|" + g_currentUsername;
    std::string response;

    if (sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, message;

        std::getline(iss, status, '|');
        std::getline(iss, message, '|');

        if (status == "LOGOUT_SUCCESS")
        {
            g_isLoggedIn = false;
            g_currentUsername = "";
            g_currentUserType = "";
            std::cout << "已退出登录: " << message << std::endl;
        }
        else
        {
            std::cout << "登出失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void handleSearchProducts()
{
    if (!g_isLoggedIn)
    {
        std::cout << "请先登录。" << std::endl;
        return;
    }

    std::string searchTerm;
    std::cout << "输入搜索名称: ";
    std::getline(std::cin >> std::ws, searchTerm);

    std::string request = "SEARCH_PRODUCTS|" + searchTerm;
    std::string response;

    if (sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, countStr;

        std::getline(iss, status, '|');

        if (status == "SEARCH_RESULTS")
        {
            std::getline(iss, countStr, '|');
            int count = std::stoi(countStr);

            if (count == 0)
            {
                std::cout << "未找到匹配商品。" << std::endl;
            }
            else
            {
                std::cout << "\n--- 搜索结果 ---" << std::endl;

                for (int i = 0; i < count; i++)
                {
                    std::string item;
                    std::getline(iss, item, '|');

                    std::istringstream itemStream(item);
                    std::string name, type, priceStr, quantityStr, seller, description;

                    std::getline(itemStream, name, ',');
                    std::getline(itemStream, type, ',');
                    std::getline(itemStream, priceStr, ',');
                    std::getline(itemStream, quantityStr, ',');
                    std::getline(itemStream, seller, ',');
                    std::getline(itemStream, description, ',');

                    std::cout << "-----------------" << std::endl;
                    std::cout << "商品名称: " << name << std::endl;
                    std::cout << "类型: " << type << std::endl;
                    std::cout << "价格: " << priceStr << " 元" << std::endl;
                    std::cout << "库存: " << quantityStr << std::endl;
                    std::cout << "商家: " << seller << std::endl;
                    std::cout << "描述: " << description << std::endl;
                }
                std::cout << "-----------------" << std::endl;
            }
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "搜索失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void handleDirectPurchase()
{
    if (!g_isLoggedIn || g_currentUserType != "customer")
    {
        std::cout << "请先以消费者身份登录后再购买商品！" << std::endl;
        return;
    }

    std::string productName, sellerName;
    int quantity;

    std::cout << "\n===== 直接购买商品 =====\n";
    std::cout << "输入要购买的商品名称: ";
    std::getline(std::cin >> std::ws, productName);
    std::cout << "输入商家名称: ";
    std::getline(std::cin >> std::ws, sellerName);

    // 先获取商品详情
    std::string detailRequest = "GET_PRODUCT_DETAIL|" + productName + "|" + sellerName;
    std::string detailResponse;

    if (!sendRequest(detailRequest, detailResponse))
    {
        std::cout << "与服务器通信失败，无法获取商品详情。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    std::istringstream detailIss(detailResponse);
    std::string detailStatus;
    std::getline(detailIss, detailStatus, '|');

    if (detailStatus != "PRODUCT_DETAIL")
    {
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

    // 输入购买数量
    std::cout << "输入购买数量: ";
    if (!(std::cin >> quantity) || quantity <= 0)
    {
        std::cout << "无效购买数量！" << std::endl;
        clearInputBuffer();
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }
    clearInputBuffer();

    // 检查库存
    if (stock < quantity)
    {
        std::cout << "库存不足！当前库存: " << stock << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 检查余额
    double totalCost = price * quantity;

    // 获取当前余额
    std::string balanceRequest = "CHECK_BALANCE|" + g_currentUsername;
    std::string balanceResponse;
    double balance = 0.0;

    if (sendRequest(balanceRequest, balanceResponse))
    {
        std::istringstream balanceIss(balanceResponse);
        std::string balanceStatus, balanceStr;
        std::getline(balanceIss, balanceStatus, '|');

        if (balanceStatus == "BALANCE")
        {
            std::getline(balanceIss, balanceStr, '|');
            balance = std::stod(balanceStr);
        }
    }

    if (balance < totalCost)
    {
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

    if (tolower(confirmChoice) != 'y')
    {
        std::cout << "订单已取消。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    } // 先获取商品详情
    detailRequest = "GET_PRODUCT_DETAIL|" + productName + "|" + sellerName;
    // std::string detailResponse;

    if (!sendRequest(detailRequest, detailResponse))
    {
        std::cout << "与服务器通信失败，无法获取商品详情。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    detailIss = std::istringstream(detailResponse);
    // std::string detailStatus;
    std::getline(detailIss, detailStatus, '|');

    if (detailStatus != "PRODUCT_DETAIL")
    {
        std::string errorMsg;
        std::getline(detailIss, errorMsg, '|');
        std::cout << "未找到商品: " << errorMsg << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 商品存在，获取价格和库存
    // std::string name, type, description, priceStr, quantityStr, seller;
    std::getline(detailIss, name, '|');
    std::getline(detailIss, type, '|');
    std::getline(detailIss, description, '|');
    std::getline(detailIss, priceStr, '|');
    std::getline(detailIss, quantityStr, '|');
    std::getline(detailIss, seller, '|');

    price = std::stod(priceStr);
    stock = std::stoi(quantityStr);

    // 检查库存
    if (stock < quantity)
    {
        std::cout << "库存不足！当前库存: " << stock << std::endl;
        std::cout << "购买失败！按回车键继续...";
        std::cin.get();
        return;
    }

    // 提交直接购买请求
    std::string purchaseRequest = "DIRECT_PURCHASE|" + g_currentUsername + "|" + productName + "|" + sellerName + "|" + std::to_string(quantity);
    std::string purchaseResponse;

    std::cout << "正在处理订单，请稍候..." << std::endl;

    if (sendRequest(purchaseRequest, purchaseResponse))
    {
        std::istringstream purchaseIss(purchaseResponse);
        std::string purchaseStatus, message, orderId;

        std::getline(purchaseIss, purchaseStatus, '|');
        std::getline(purchaseIss, message, '|');

        if (purchaseStatus == "PURCHASE_SUCCESS")
        {
            std::getline(purchaseIss, orderId, '|');
            std::cout << "购买成功! 订单ID: " << orderId << std::endl;
            std::cout << message << std::endl;
        }
        else if (purchaseStatus == "PURCHASE_PROCESSING")
        {
            std::getline(purchaseIss, orderId, '|');
            std::cout << "订单已提交，正在处理中... 订单ID: " << orderId << std::endl;
            std::cout << "请稍等，您可以稍后查询订单状态。" << std::endl;
        }
        else if (purchaseStatus == "PURCHASE_FAILED")
        {
            std::cout << "购买失败: " << message << std::endl;
        }
        else
        {
            std::cout << "未知错误，请联系客服。" << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}
