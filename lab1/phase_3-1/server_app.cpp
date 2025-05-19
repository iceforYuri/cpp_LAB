#include "server_app.h"
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
#include <fstream>

#include "user/user.h"
#include "store/store.h"
#include "order/order.h"
#include "ordermanager/ordermanager.h"

#pragma comment(lib, "ws2_32.lib")

// 构造函数
ServerApp::ServerApp() 
    : serverSocket(INVALID_SOCKET),
      serverRunning(true),
      orderManager(*(new OrderManager("./order/orders"))),
      USER_FILE("./user/users.txt"),
      STORE_FILE("./store"),
      CART_FILE("./user/carts"),
      ORDER_DIR("./order/orders") {
    
    // 初始化工作在 initialize() 方法中完成
}

// 析构函数
ServerApp::~ServerApp() {
    // 关闭服务器
    shutdown();
    
    // 保存用户数据
    saveUsers();
    
    // 保存商店数据
    saveStore();
    
    // 释放用户对象内存
    for (auto user : users) {
        delete user;
    }
    users.clear();
}

// 初始化服务器
bool ServerApp::initialize() {
    // 初始化 Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup 失败: " << WSAGetLastError() << std::endl;
        return false;
    }
    
    // 创建服务器 socket
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "创建 socket 失败: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }
    
    // 设置服务器地址
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888); // 使用 8888 端口
    
    // 绑定 socket
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "绑定失败: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return false;
    }
    
    // 开始监听
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "监听失败: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return false;
    }
    
    // 加载用户数据
    loadUsers();
    
    // 加载商店数据
    loadStore();
    
    std::cout << "服务器初始化成功，监听端口: 8888" << std::endl;
    return true;
}

// 加载用户数据
void ServerApp::loadUsers() {
    // 清空现有用户
    for (auto user : users) {
        delete user;
    }
    users.clear();
    
    // 加载用户
    std::vector<User*> loadedUsers = User::loadUsersFromFile(USER_FILE);
    users = loadedUsers;
    
    std::cout << "已加载 " << users.size() << " 个用户。" << std::endl;
}

// 保存用户数据
void ServerApp::saveUsers() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(usersMutex));
    User::saveUsersToFile(users, USER_FILE);
    std::cout << "用户数据已保存。" << std::endl;
}

// 加载商店数据
void ServerApp::loadStore() {
    // 加载商店
    store = Store(STORE_FILE);
    std::cout << "商店数据已加载。" << std::endl;
}

// 保存商店数据
void ServerApp::saveStore() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(storeMutex));
    store.saveProducts();
    std::cout << "商店数据已保存。" << std::endl;
}

// 运行服务器
void ServerApp::run() {
    std::cout << "服务器启动中..." << std::endl;
    
    if (!initialize()) {
        std::cerr << "服务器初始化失败。" << std::endl;
        return;
    }
    
    std::cout << "服务器已启动，等待客户端连接..." << std::endl;
    
    // 主循环，接受客户端连接
    while (serverRunning) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        
        // 接受新的客户端连接
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            if (!serverRunning) {
                // 服务器已关闭，跳出循环
                break;
            }
            std::cerr << "接受连接失败: " << WSAGetLastError() << std::endl;
            continue;
        }
        
        // 获取客户端 IP 地址
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        std::cout << "客户端已连接: " << clientIP << std::endl;
        
        // 创建新线程处理客户端请求
        clientThreads.push_back(std::thread(&ServerApp::handleClient, this, clientSocket));
    }
    
    // 等待所有客户端线程结束
    for (auto &thread : clientThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    std::cout << "服务器已关闭。" << std::endl;
}

// 处理客户端请求
void ServerApp::handleClient(SOCKET clientSocket) {
    char buffer[4096] = {0};
    std::string response;
    bool clientConnected = true;
    std::string currentConnectedUser = ""; // 跟踪当前连接的用户名
    
    std::cout << "客户端已连接，处理中..." << std::endl;
    
    while (clientConnected && serverRunning) {
        // 清空缓冲区
        memset(buffer, 0, sizeof(buffer));
        
        // 接收客户端请求
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            std::cout << "客户端断开连接或接收错误" << std::endl;
            break;
        }
        
        // 解析请求
        std::string request(buffer);
        std::cout << "收到请求: " << request << std::endl;
        
        // 处理请求的核心逻辑
        std::istringstream iss(request);
        std::string command;
        std::getline(iss, command, '|');
        
        // 根据命令类型处理请求
        if (command == "LOGIN") {
            std::string username, password;
            std::getline(iss, username, '|');
            std::getline(iss, password, '|');
            
            response = handleLoginRequest(username, password);
            
            // 如果登录成功，记录当前连接的用户
            if (response.substr(0, 13) == "LOGIN_SUCCESS") {
                std::istringstream respIss(response);
                std::string status, user;
                std::getline(respIss, status, '|');
                std::getline(respIss, user, '|');
                currentConnectedUser = user;
            }
        }
        else if (command == "REGISTER") {
            std::string username, password, type;
            std::getline(iss, username, '|');
            std::getline(iss, password, '|');
            std::getline(iss, type, '|');
            
            response = handleRegisterRequest(username, password, type);
        }
        else if (command == "LOGOUT") {
            std::string username;
            std::getline(iss, username, '|');
            
            response = handleLogoutRequest(username);
            
            // 如果登出成功，清除当前连接的用户
            if (response.substr(0, 14) == "LOGOUT_SUCCESS") {
                currentConnectedUser = "";
            }
        }
        else if (command == "GET_PRODUCTS") {
            response = handleGetProductsRequest();
        }
        else if (command == "GET_PRODUCT_DETAIL") {
            std::string productName, sellerUsername;
            std::getline(iss, productName, '|');
            std::getline(iss, sellerUsername, '|');
            
            response = handleGetProductDetailRequest(productName, sellerUsername);
        }
        else if (command == "SEARCH_PRODUCTS") {
            std::string searchType, searchTerm;
            std::getline(iss, searchType, '|');
            
            if (searchType == "price") {
                // 按价格区间搜索
                std::string minPriceStr, maxPriceStr;
                std::getline(iss, minPriceStr, '|');
                std::getline(iss, maxPriceStr, '|');
                
                double minPrice = std::stod(minPriceStr);
                double maxPrice = std::stod(maxPriceStr);
                
                response = handleSearchProductsByPriceRequest(minPrice, maxPrice);
            } else {
                // 按名称或类型搜索
                std::getline(iss, searchTerm, '|');
                response = handleSearchProductsRequest(searchType, searchTerm);
            }
        }
        else if (command == "GET_USER_INFO") {
            std::string username;
            std::getline(iss, username, '|');
            
            response = handleGetUserInfoRequest(username);
        }
        else if (command == "CHANGE_PASSWORD") {
            std::string username, oldPassword, newPassword;
            std::getline(iss, username, '|');
            std::getline(iss, oldPassword, '|');
            std::getline(iss, newPassword, '|');
            
            response = handleChangePasswordRequest(username, oldPassword, newPassword);
        }
        else if (command == "CHECK_BALANCE") {
            std::string username;
            std::getline(iss, username, '|');
            
            response = handleCheckBalanceRequest(username);
        }
        else if (command == "DEPOSIT") {
            std::string username, amountStr;
            std::getline(iss, username, '|');
            std::getline(iss, amountStr, '|');
            
            double amount = std::stod(amountStr);
            
            response = handleDepositRequest(username, amount);
        }
        else if (command == "DIRECT_PURCHASE") {
            std::string username, productName, sellerName, quantityStr;
            std::getline(iss, username, '|');
            std::getline(iss, productName, '|');
            std::getline(iss, sellerName, '|');
            std::getline(iss, quantityStr, '|');
            
            int quantity = std::stoi(quantityStr);
            
            response = handleDirectPurchaseRequest(username, productName, sellerName, quantity);
        }
        else if (command == "VIEW_CART") {
            std::string username;
            std::getline(iss, username, '|');
            
            response = handleViewCartRequest(username);
        }
        else if (command == "ADD_TO_CART") {
            std::string username, productName, sellerName, quantityStr;
            std::getline(iss, username, '|');
            std::getline(iss, productName, '|');
            std::getline(iss, sellerName, '|');
            std::getline(iss, quantityStr, '|');
            
            int quantity = std::stoi(quantityStr);
            
            response = handleAddToCartRequest(username, productName, sellerName, quantity);
        }
        else if (command == "CHECKOUT") {
            std::string username;
            std::getline(iss, username, '|');
            
            response = handleCheckoutRequest(username);
        }
        else if (command == "GET_SELLER_INFO") {
            std::string username;
            std::getline(iss, username, '|');
            
            response = handleSellerInfoRequest(username);
        }
        else if (command == "CHECK_INCOME") {
            std::string username;
            std::getline(iss, username, '|');
            
            response = handleCheckIncomeRequest(username);
        }
        else if (command == "ADD_PRODUCT") {
            std::string sellerUsername, productInfo;
            std::getline(iss, sellerUsername, '|');
            std::getline(iss, productInfo, '|');
            
            response = handleAddProductRequest(sellerUsername, productInfo);
        }
        else if (command == "UPDATE_PRODUCT") {
            std::string sellerUsername, productName, updateInfo;
            std::getline(iss, sellerUsername, '|');
            std::getline(iss, productName, '|');
            std::getline(iss, updateInfo, '|');
            
            response = handleUpdateProductRequest(sellerUsername, productName, updateInfo);
        }
        else if (command == "REMOVE_PRODUCT") {
            std::string sellerUsername, productName;
            std::getline(iss, sellerUsername, '|');
            std::getline(iss, productName, '|');
            
            response = handleRemoveProductRequest(sellerUsername, productName);
        }
        else {
            response = "ERROR|未知命令";
        }
        
        // 发送响应
        if (send(clientSocket, response.c_str(), response.size(), 0) == SOCKET_ERROR) {
            std::cerr << "发送响应失败: " << WSAGetLastError() << std::endl;
            break;
        }
    }
    
    // 如果用户已登录但连接断开，需要登出该用户
    if (!currentConnectedUser.empty()) {
        logoutUser(currentConnectedUser);
        std::cout << "用户 " << currentConnectedUser << " 已登出。" << std::endl;
    }
    
    // 关闭客户端 socket
    closesocket(clientSocket);
    std::cout << "客户端连接已关闭。" << std::endl;
}

// 关闭服务器
void ServerApp::shutdown() {
    std::cout << "正在关闭服务器..." << std::endl;
    
    // 标记服务器为关闭状态
    serverRunning = false;
    
    // 关闭服务器 socket
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
    
    // 清理 Winsock
    WSACleanup();
}

// 用户登录
User* ServerApp::loginUser(const std::string& username, const std::string& password) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(usersMutex));
    return User::login(users, username, password);
}

// 用户登出
bool ServerApp::logoutUser(const std::string& username) {
    std::lock_guard<std::mutex> lock(loggedInUsersMutex);
    auto it = loggedInUsers.find(username);
    if (it != loggedInUsers.end()) {
        loggedInUsers.erase(it);
        return true;
    }
    return false;
}

// 检查用户是否已登录
bool ServerApp::isUserLoggedIn(const std::string& username) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(loggedInUsersMutex));
    return loggedInUsers.find(username) != loggedInUsers.end();
}

// 注册新用户
bool ServerApp::registerUser(const std::string& username, const std::string& password, const std::string& type) {
    std::lock_guard<std::mutex> lock(usersMutex);
    
    // 检查用户名是否已存在
    if (User::isUsernameExists(users, username)) {
        return false;
    }
    
    // 创建新用户
    User* newUser = nullptr;
    if (type == "customer") {
        newUser = new Customer(username, password, CART_FILE, 0.0);
    } else if (type == "seller") {
        newUser = new Seller(username, password, 0.0);
    } else {
        return false; // 未知的用户类型
    }
    
    // 添加到用户列表
    if (newUser) {
        users.push_back(newUser);
        User::saveUsersToFile(users, USER_FILE);
        return true;
    }
    
    return false;
}

// 处理登录请求
std::string ServerApp::handleLoginRequest(const std::string& username, const std::string& password) {
    // 检查用户是否已经登录
    if (isUserLoggedIn(username)) {
        return "LOGIN_FAILED|该用户已在其他客户端登录";
    }
    
    // 尝试登录
    User* user = loginUser(username, password);
    if (user) {
        // 添加到已登录用户集合
        std::lock_guard<std::mutex> lock(loggedInUsersMutex);
        loggedInUsers.insert(username);
        
        return "LOGIN_SUCCESS|" + user->getUsername() + "|" + user->getUserType();
    } else {
        return "LOGIN_FAILED|用户名或密码错误";
    }
}

// 处理注册请求
std::string ServerApp::handleRegisterRequest(const std::string& username, const std::string& password, const std::string& type) {
    if (registerUser(username, password, type)) {
        return "REGISTER_SUCCESS|" + username;
    } else {
        return "REGISTER_FAILED|用户名已存在或创建用户失败";
    }
}

// 处理登出请求
std::string ServerApp::handleLogoutRequest(const std::string& username) {
    if (logoutUser(username)) {
        return "LOGOUT_SUCCESS|用户已成功登出";
    } else {
        return "LOGOUT_FAILED|用户未登录或登出失败";
    }
}

// 处理获取商品列表请求
std::string ServerApp::handleGetProductsRequest() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(storeMutex));
    const auto& products = store.getProducts();
    
    std::ostringstream oss;
    oss << "PRODUCTS|" << products.size();
    
    for (const auto& product : products) {
        oss << "|" << product->getName()
            << "," << product->getType()
            << "," << product->getPrice()
            << "," << product->getQuantity()
            << "," << product->getSellerUsername();
    }
    
    return oss.str();
}

// 处理获取商品详情请求
std::string ServerApp::handleGetProductDetailRequest(const std::string& productName, const std::string& sellerUsername) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(storeMutex));
    Product* product = store.findProductByName(productName, sellerUsername);
    
    if (product) {
        std::ostringstream oss;
        oss << "PRODUCT_DETAIL|" << product->getName()
            << "|" << product->getType()
            << "|" << product->getDescription()
            << "|" << product->getPrice()
            << "|" << product->getQuantity()
            << "|" << product->getSellerUsername();
        
        // 根据不同类型添加特定属性
        if (product->getType() == "Book") {
            Book* book = dynamic_cast<Book*>(product);
            if (book) {
                oss << "|" << book->getAuthor() << "|" << book->getIsbn();
            }
        } else if (product->getType() == "Clothing") {
            Clothing* clothing = dynamic_cast<Clothing*>(product);
            if (clothing) {
                oss << "|" << clothing->getSize() << "|" << clothing->getMaterial();
            }
        } else if (product->getType() == "Food") {
            Food* food = dynamic_cast<Food*>(product);
            if (food) {
                oss << "|" << food->getExpiryDate();
            }
        } else if (product->getType() == "Electronics") {
            Electronics* electronics = dynamic_cast<Electronics*>(product);
            if (electronics) {
                oss << "|" << electronics->getBrand() << "|" << electronics->getModel();
            }
        }
        
        return oss.str();
    } else {
        return "ERROR|未找到商品";
    }
}

// 处理搜索商品请求
std::string ServerApp::handleSearchProductsRequest(const std::string& searchType, const std::string& searchTerm) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(storeMutex));
    std::vector<Product*> results;
    
    if (searchType == "name") {
        results = store.searchProductsByName(searchTerm);
    } else if (searchType == "type") {
        results = store.searchProductsByType(searchTerm);
    } else {
        return "ERROR|无效的搜索类型";
    }
    
    std::ostringstream oss;
    oss << "SEARCH_RESULTS|" << results.size();
    
    for (const auto& product : results) {
        oss << "|" << product->getName()
            << "," << product->getType()
            << "," << product->getPrice()
            << "," << product->getQuantity()
            << "," << product->getSellerUsername();
    }
    
    return oss.str();
}

// 处理按价格区间搜索商品请求
std::string ServerApp::handleSearchProductsByPriceRequest(double minPrice, double maxPrice) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(storeMutex));
    std::vector<Product*> results = store.searchProductsByPriceRange(minPrice, maxPrice);
    
    std::ostringstream oss;
    oss << "SEARCH_RESULTS|" << results.size();
    
    for (const auto& product : results) {
        oss << "|" << product->getName()
            << "," << product->getType()
            << "," << product->getPrice()
            << "," << product->getQuantity()
            << "," << product->getSellerUsername();
    }
    
    return oss.str();
}

// 处理获取用户信息请求
std::string ServerApp::handleGetUserInfoRequest(const std::string& username) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(usersMutex));
    
    // 查找用户
    User* user = nullptr;
    for (auto u : users) {
        if (u->getUsername() == username) {
            user = u;
            break;
        }
    }
    
    if (user) {
        std::ostringstream oss;
        oss << "USER_INFO|" << user->getUsername()
            << "|" << user->getUserType()
            << "|" << user->getBalance()
            << "|" << user->getCreationTime();
        
        return oss.str();
    } else {
        return "ERROR|未找到用户";
    }
}

// 处理修改密码请求
std::string ServerApp::handleChangePasswordRequest(const std::string& username, const std::string& oldPassword, const std::string& newPassword) {
    std::lock_guard<std::mutex> lock(usersMutex);
    
    // 查找用户
    User* user = nullptr;
    for (auto u : users) {
        if (u->getUsername() == username) {
            user = u;
            break;
        }
    }
    
    if (user) {
        if (user->getPassword() == oldPassword) {
            user->setPassword(newPassword);
            User::saveUsersToFile(users, USER_FILE);
            return "PASSWORD_CHANGED|密码已成功修改";
        } else {
            return "PASSWORD_CHANGE_FAILED|旧密码不正确";
        }
    } else {
        return "PASSWORD_CHANGE_FAILED|未找到用户";
    }
}

// 处理查询余额请求
std::string ServerApp::handleCheckBalanceRequest(const std::string& username) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(usersMutex));
    
    // 查找用户
    User* user = nullptr;
    for (auto u : users) {
        if (u->getUsername() == username) {
            user = u;
            break;
        }
    }
    
    if (user) {
        return "BALANCE|" + std::to_string(user->getBalance());
    } else {
        return "ERROR|未找到用户";
    }
}

// 处理充值请求
std::string ServerApp::handleDepositRequest(const std::string& username, double amount) {
    std::lock_guard<std::mutex> lock(usersMutex);
    
    // 查找用户
    User* user = nullptr;
    for (auto u : users) {
        if (u->getUsername() == username) {
            user = u;
            break;
        }
    }
    
    if (user) {
        if (amount <= 0) {
            return "DEPOSIT_FAILED|充值金额必须大于0";
        }
        
        double newBalance = user->getBalance() + amount;
        user->setBalance(newBalance);
        User::saveUsersToFile(users, USER_FILE);
        
        return "DEPOSIT_SUCCESS|充值成功|" + std::to_string(newBalance);
    } else {
        return "DEPOSIT_FAILED|未找到用户";
    }
}

// 处理直接购买请求
std::string ServerApp::handleDirectPurchaseRequest(const std::string& username, const std::string& productName, const std::string& sellerName, int quantity) {
    // 需要同时锁定用户和商店
    std::lock_guard<std::mutex> userLock(usersMutex);
    std::lock_guard<std::mutex> storeLock(storeMutex);
    
    // 查找用户
    User* user = nullptr;
    for (auto u : users) {
        if (u->getUsername() == username) {
            user = u;
            break;
        }
    }
    
    if (!user || user->getUserType() != "customer") {
        return "PURCHASE_FAILED|无效的用户或用户类型";
    }
    
    Customer* customer = dynamic_cast<Customer*>(user);
    if (!customer) {
        return "PURCHASE_FAILED|用户类型转换失败";
    }
    
    // 查找商品
    Product* product = store.findProductByName(productName, sellerName);
    if (!product) {
        return "PURCHASE_FAILED|未找到商品";
    }
    
    // 查找卖家
    User* seller = nullptr;
    for (auto u : users) {
        if (u->getUsername() == sellerName && u->getUserType() == "seller") {
            seller = u;
            break;
        }
    }
    
    if (!seller) {
        return "PURCHASE_FAILED|未找到卖家";
    }
    
    Seller* sellerObj = dynamic_cast<Seller*>(seller);
    if (!sellerObj) {
        return "PURCHASE_FAILED|卖家类型转换失败";
    }
    
    // 检查库存
    if (product->getQuantity() < quantity) {
        return "PURCHASE_FAILED|库存不足";
    }
    
    // 检查余额
    double totalCost = product->getPrice() * quantity;
    if (customer->getBalance() < totalCost) {
        return "PURCHASE_FAILED|余额不足";
    }
    
    // 创建订单
    Order order(orderManager.generateOrderId(), username, sellerName, productName, quantity, product->getPrice());
    std::string orderId = order.getOrderId();
    
    // 更新用户余额
    customer->setBalance(customer->getBalance() - totalCost);
    
    // 更新卖家收入
    sellerObj->setBalance(sellerObj->getBalance() + totalCost);
    
    // 更新商品库存
    product->setQuantity(product->getQuantity() - quantity);
    
    // 保存订单
    orderManager.addOrder(order);
    
    // 保存用户和商店数据
    User::saveUsersToFile(users, USER_FILE);
    store.saveProducts();
    
    return "PURCHASE_SUCCESS|购买成功|" + orderId;
}

// 处理查看购物车请求
std::string ServerApp::handleViewCartRequest(const std::string& username) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(usersMutex));
    
    // 查找用户
    User* user = nullptr;
    for (auto u : users) {
        if (u->getUsername() == username && u->getUserType() == "customer") {
            user = u;
            break;
        }
    }
    
    if (!user) {
        return "ERROR|未找到用户或用户类型不正确";
    }
    
    Customer* customer = dynamic_cast<Customer*>(user);
    if (!customer) {
        return "ERROR|用户类型转换失败";
    }
    
    const auto& cart = customer->getCart();
    std::ostringstream oss;
    oss << "CART|" << cart.size();
    
    std::lock_guard<std::mutex> storeLock(const_cast<std::mutex&>(storeMutex));
    
    for (const auto& item : cart) {
        // 查找商品获取最新价格
        Product* product = store.findProductByName(item.productName, item.sellerUsername);
        double price = product ? product->getPrice() : 0.0;
        
        oss << "|" << item.productName
            << "," << item.sellerUsername
            << "," << price
            << "," << item.quantity;
    }
    
    return oss.str();
}

// 处理添加到购物车请求
std::string ServerApp::handleAddToCartRequest(const std::string& username, const std::string& productName, const std::string& sellerName, int quantity) {
    // 需要同时锁定用户和商店
    std::lock_guard<std::mutex> userLock(usersMutex);
    std::lock_guard<std::mutex> storeLock(storeMutex);
    
    // 查找用户
    User* user = nullptr;
    for (auto u : users) {
        if (u->getUsername() == username && u->getUserType() == "customer") {
            user = u;
            break;
        }
    }
    
    if (!user) {
        return "CART_UPDATE_FAILED|未找到用户或用户类型不正确";
    }
    
    Customer* customer = dynamic_cast<Customer*>(user);
    if (!customer) {
        return "CART_UPDATE_FAILED|用户类型转换失败";
    }
    
    // 查找商品
    Product* product = store.findProductByName(productName, sellerName);
    if (!product) {
        return "CART_UPDATE_FAILED|未找到商品";
    }
    
    // 检查库存
    if (product->getQuantity() < quantity) {
        return "CART_UPDATE_FAILED|库存不足";
    }
    
    // 添加到购物车
    bool success = customer->addToCart(productName, sellerName, quantity);
    
    if (success) {
        return "CART_UPDATED|商品已添加到购物车";
    } else {
        return "CART_UPDATE_FAILED|添加到购物车失败";
    }
}

// 处理结算购物车请求
std::string ServerApp::handleCheckoutRequest(const std::string& username) {
    // 需要同时锁定用户和商店
    std::lock_guard<std::mutex> userLock(usersMutex);
    std::lock_guard<std::mutex> storeLock(storeMutex);
    
    // 查找用户
    User* user = nullptr;
    for (auto u : users) {
        if (u->getUsername() == username && u->getUserType() == "customer") {
            user = u;
            break;
        }
    }
    
    if (!user) {
        return "CHECKOUT_FAILED|未找到用户或用户类型不正确";
    }
    
    Customer* customer = dynamic_cast<Customer*>(user);
    if (!customer) {
        return "CHECKOUT_FAILED|用户类型转换失败";
    }
    
    // 获取购物车
    const auto& cart = customer->getCart();
    if (cart.empty()) {
        return "CHECKOUT_FAILED|购物车为空";
    }
    
    // 计算总价并检查库存
    double totalCost = 0.0;
    std::vector<std::pair<Product*, int>> productsToUpdate;
    std::vector<std::pair<Seller*, double>> sellersToUpdate;
    
    for (const auto& item : cart) {
        // 查找商品
        Product* product = store.findProductByName(item.productName, item.sellerUsername);
        if (!product) {
            return "CHECKOUT_FAILED|商品 " + item.productName + " 不存在或已下架";
        }
        
        // 检查库存
        if (product->getQuantity() < item.quantity) {
            return "CHECKOUT_FAILED|商品 " + item.productName + " 库存不足";
        }
        
        // 查找卖家
        User* sellerUser = nullptr;
        for (auto u : users) {
            if (u->getUsername() == item.sellerUsername && u->getUserType() == "seller") {
                sellerUser = u;
                break;
            }
        }
        
        if (!sellerUser) {
            return "CHECKOUT_FAILED|卖家 " + item.sellerUsername + " 不存在";
        }
        
        Seller* seller = dynamic_cast<Seller*>(sellerUser);
        if (!seller) {
            return "CHECKOUT_FAILED|卖家类型转换失败";
        }
        
        double itemCost = product->getPrice() * item.quantity;
        totalCost += itemCost;
        
        productsToUpdate.push_back({product, item.quantity});
        
        // 检查是否已添加该卖家
        bool sellerFound = false;
        for (auto& sellerPair : sellersToUpdate) {
            if (sellerPair.first == seller) {
                sellerPair.second += itemCost;
                sellerFound = true;
                break;
            }
        }
        
        if (!sellerFound) {
            sellersToUpdate.push_back({seller, itemCost});
        }
    }
    
    // 检查余额
    if (customer->getBalance() < totalCost) {
        return "CHECKOUT_FAILED|余额不足";
    }
    
    // 创建订单
    std::string orderId = orderManager.generateOrderId();
    
    // 扣除用户余额
    customer->setBalance(customer->getBalance() - totalCost);
    
    // 更新卖家收入
    for (auto& sellerPair : sellersToUpdate) {
        sellerPair.first->setBalance(sellerPair.first->getBalance() + sellerPair.second);
    }
    
    // 更新商品库存
    for (auto& productPair : productsToUpdate) {
        productPair.first->setQuantity(productPair.first->getQuantity() - productPair.second);
    }
    
    // 清空购物车
    customer->clearCart();
    
    // 保存用户和商店数据
    User::saveUsersToFile(users, USER_FILE);
    store.saveProducts();
    
    return "CHECKOUT_SUCCESS|结算成功，总金额: " + std::to_string(totalCost) + " 元|" + orderId;
}

// 处理获取商家信息请求
std::string ServerApp::handleSellerInfoRequest(const std::string& username) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(usersMutex));
    
    // 查找用户
    User* user = nullptr;
    for (auto u : users) {
        if (u->getUsername() == username && u->getUserType() == "seller") {
            user = u;
            break;
        }
    }
    
    if (!user) {
        return "ERROR|未找到用户或用户类型不正确";
    }
    
    Seller* seller = dynamic_cast<Seller*>(user);
    if (!seller) {
        return "ERROR|用户类型转换失败";
    }
    
    // 计算在售商品数量
    std::lock_guard<std::mutex> storeLock(const_cast<std::mutex&>(storeMutex));
    int productCount = 0;
    for (const auto& product : store.getProducts()) {
        if (product->getSellerUsername() == username) {
            productCount++;
        }
    }
    
    std::ostringstream oss;
    oss << "SELLER_INFO|" << seller->getUsername()
        << "|" << seller->getBalance()
        << "|" << productCount
        << "|" << seller->getCreationTime();
    
    return oss.str();
}

// 处理查看收入请求
std::string ServerApp::handleCheckIncomeRequest(const std::string& username) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(usersMutex));
    
    // 查找用户
    User* user = nullptr;
    for (auto u : users) {
        if (u->getUsername() == username && u->getUserType() == "seller") {
            user = u;
            break;
        }
    }
    
    if (!user) {
        return "ERROR|未找到用户或用户类型不正确";
    }
    
    Seller* seller = dynamic_cast<Seller*>(user);
    if (!seller) {
        return "ERROR|用户类型转换失败";
    }
    
    return "INCOME|" + std::to_string(seller->getBalance());
}

// 处理添加商品请求
std::string ServerApp::handleAddProductRequest(const std::string& sellerUsername, const std::string& productInfo) {
    std::lock_guard<std::mutex> userLock(usersMutex);
    std::lock_guard<std::mutex> storeLock(storeMutex);
    
    // 查找用户
    User* user = nullptr;
    for (auto u : users) {
        if (u->getUsername() == sellerUsername && u->getUserType() == "seller") {
            user = u;
            break;
        }
    }
    
    if (!user) {
        return "ADD_PRODUCT_FAILED|未找到用户或用户类型不正确";
    }
    
    // 解析商品信息
    std::istringstream iss(productInfo);
    std::string type, name, description, priceStr, quantityStr;
    std::getline(iss, type, '|');
    std::getline(iss, name, '|');
    std::getline(iss, description, '|');
    std::getline(iss, priceStr, '|');
    std::getline(iss, quantityStr, '|');
    
    double price = std::stod(priceStr);
    int quantity = std::stoi(quantityStr);
    
    // 检查商品是否已存在
    if (store.findProductByName(name, sellerUsername)) {
        return "ADD_PRODUCT_FAILED|同名商品已存在";
    }
    
    // 根据类型创建商品
    Product* product = nullptr;
    
    if (type == "Book") {
        std::string author, isbn;
        std::getline(iss, author, '|');
        std::getline(iss, isbn, '|');
        
        product = new Book(name, description, price, quantity, sellerUsername, author, isbn);
    } else if (type == "Clothing") {
        std::string size, material;
        std::getline(iss, size, '|');
        std::getline(iss, material, '|');
        
        product = new Clothing(name, description, price, quantity, sellerUsername, size, material);
    } else if (type == "Food") {
        std::string expiryDate;
        std::getline(iss, expiryDate, '|');
        
        product = new Food(name, description, price, quantity, sellerUsername, expiryDate);
    } else if (type == "Electronics") {
        std::string brand, model;
        std::getline(iss, brand, '|');
        std::getline(iss, model, '|');
        
        product = new Electronics(name, description, price, quantity, sellerUsername, brand, model);
    } else {
        // 未知类型，创建基本商品
        product = new Product(name, type, description, price, quantity, sellerUsername);
    }
    
    if (!product) {
        return "ADD_PRODUCT_FAILED|创建商品失败";
    }
    
    // 添加商品
    store.addProduct(product);
    store.saveProducts();
    
    return "ADD_PRODUCT_SUCCESS|商品已成功添加";
}

// 处理更新商品请求
std::string ServerApp::handleUpdateProductRequest(const std::string& sellerUsername, const std::string& productName, const std::string& updateInfo) {
    std::lock_guard<std::mutex> storeLock(storeMutex);
    
    // 查找商品
    Product* product = store.findProductByName(productName, sellerUsername);
    if (!product) {
        return "UPDATE_PRODUCT_FAILED|未找到商品";
    }
    
    // 检查商品是否属于该卖家
    if (product->getSellerUsername() != sellerUsername) {
        return "UPDATE_PRODUCT_FAILED|无权修改该商品";
    }
    
    // 解析更新信息
    std::istringstream iss(updateInfo);
    std::string field, value;
    std::getline(iss, field, '|');
    std::getline(iss, value, '|');
    
    // 根据字段更新商品
    if (field == "description") {
        product->setDescription(value);
    } else if (field == "price") {
        double newPrice = std::stod(value);
        if (newPrice <= 0) {
            return "UPDATE_PRODUCT_FAILED|价格必须大于0";
        }
        product->setPrice(newPrice);
    } else if (field == "quantity") {
        int newQuantity = std::stoi(value);
        if (newQuantity < 0) {
            return "UPDATE_PRODUCT_FAILED|库存不能为负数";
        }
        product->setQuantity(newQuantity);
    } else {
        return "UPDATE_PRODUCT_FAILED|无效的更新字段";
    }
    
    store.saveProducts();
    
    return "UPDATE_PRODUCT_SUCCESS|商品已成功更新";
}

// 处理移除商品请求
std::string ServerApp::handleRemoveProductRequest(const std::string& sellerUsername, const std::string& productName) {
    std::lock_guard<std::mutex> storeLock(storeMutex);
    
    // 查找商品
    Product* product = store.findProductByName(productName, sellerUsername);
    if (!product) {
        return "REMOVE_PRODUCT_FAILED|未找到商品";
    }
    
    // 检查商品是否属于该卖家
    if (product->getSellerUsername() != sellerUsername) {
        return "REMOVE_PRODUCT_FAILED|无权删除该商品";
    }
    
    // 移除商品
    bool success = store.removeProduct(productName, sellerUsername);
    
    if (success) {
        store.saveProducts();
        return "REMOVE_PRODUCT_SUCCESS|商品已成功移除";
    } else {
        return "REMOVE_PRODUCT_FAILED|移除商品失败";
    }
}
