#include "server.h"
#include <iostream>
#include <windows.h>

// 服务器类构造函数
Server::Server()
    : serverSocket(INVALID_SOCKET),
      serverRunning(true),
      USER_FILE("./user/users.txt"),
      STORE_FILE("./store"),
      CART_FILE("./user/carts"),
      ORDER_DIR("./order/orders"),
      store(STORE_FILE),
      orderManager(ORDER_DIR) {
}

// 服务器类析构函数
Server::~Server() {
    // 停止服务器
    stop();
    
    // 清理资源
    if (serverSocket != INVALID_SOCKET) {
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
    
    // 释放用户对象
    for (auto* user : users) {
        delete user;
    }
    
    // 清理Winsock
    WSACleanup();
}

// 初始化服务器
bool Server::initialize() {
    // 设置控制台编码
    SetConsoleOutputCP(65001); // 设置控制台输出编码为 UTF-8
    SetConsoleCP(65001);       // 设置控制台输入编码为 UTF-8

    // 初始化 Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup 失败: " << result << std::endl;
        return false;
    }

    // 创建服务器socket
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "创建socket失败: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    // 绑定到本地地址和端口
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888); // 使用8888端口

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

    std::cout << "服务器初始化成功。" << std::endl;
    return true;
}

// 启动服务器
bool Server::start() {
    // 加载用户数据
    users = User::loadUsersFromFile(USER_FILE);
    std::cout << "已加载 " << users.size() << " 个用户数据。" << std::endl;

    // 加载商品数据
    store.loadAllProducts();
    std::cout << "已加载商品数据。" << std::endl;

    // 启动订单处理线程
    orderManager.startProcessingThread(store, users);
    std::cout << "订单处理线程已启动。" << std::endl;

    return true;
}

// 停止服务器
void Server::stop() {
    // 设置服务器停止标志
    serverRunning = false;
    
    // 停止订单处理线程
    orderManager.stopProcessingThread();
    
    // 保存用户数据
    User::saveUsersToFile(users, USER_FILE);
    std::cout << "用户数据已保存。" << std::endl;
}

// 服务器主循环
void Server::run() {
    std::cout << "服务器启动，等待连接..." << std::endl;
    
    std::vector<std::thread> clientThreads;

    // 主循环，接受客户端连接
    while (serverRunning) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "接受连接失败: " << WSAGetLastError() << std::endl;
            continue;
        }

        // 为每个客户端创建新线程
        clientThreads.emplace_back(&Server::handleClient, this, clientSocket);
    }

    // 等待所有客户端线程结束
    for (auto& thread : clientThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

// 处理客户端请求
void Server::handleClient(SOCKET clientSocket) {
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

        // 提取命令
        std::string command = request.substr(0, request.find("|"));
        response = "ERROR|未知命令";

        // 处理不同类型的请求
        if (command == "LOGIN") {
            response = handleLoginRequest(request, currentConnectedUser);
        } else if (command == "REGISTER") {
            response = handleRegisterRequest(request);
        } else if (command == "GET_PRODUCTS") {
            response = handleGetProductsRequest();
        } else if (command == "GET_PRODUCT_DETAIL") {
            response = handleGetProductDetailRequest(request);
        } else if (command == "ADD_TO_CART") {
            response = handleAddToCartRequest(request);
        } else if (command == "GET_CART") {
            response = handleGetCartRequest(request);
        } else if (command == "CHECKOUT") {
            response = handleCheckoutRequest(request);
        } else if (command == "LOGOUT") {
            response = handleLogoutRequest(request, currentConnectedUser);
        } else if (command == "GET_USER_INFO") {
            response = handleGetUserInfoRequest(request);
        } else if (command == "CHECK_BALANCE") {
            response = handleCheckBalanceRequest(request);
        } else if (command == "DEPOSIT") {
            response = handleDepositRequest(request);
        }
        // ... 处理其他命令

        // 发送响应给客户端
        send(clientSocket, response.c_str(), response.size(), 0);
    }

    // 检查此连接是否有关联的用户，如果有则从已登录用户集合中移除
    if (!currentConnectedUser.empty()) {
        loggedInUsersMutex.lock();
        auto it = loggedInUsers.find(currentConnectedUser);
        if (it != loggedInUsers.end()) {
            loggedInUsers.erase(it);
            std::cout << "用户 " << currentConnectedUser << " 已因连接断开而被自动登出" << std::endl;
        }
        loggedInUsersMutex.unlock();
    }

    // 客户端断开连接，关闭socket
    closesocket(clientSocket);
    std::cout << "客户端连接已关闭" << std::endl;
}

// 处理登录请求
std::string Server::handleLoginRequest(const std::string &request, std::string &currentConnectedUser) {
    std::istringstream iss(request);
    std::string cmd, username, password;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, password, '|');

    // 检查用户是否已经登录
    loggedInUsersMutex.lock();
    bool alreadyLoggedIn = (loggedInUsers.find(username) != loggedInUsers.end());
    loggedInUsersMutex.unlock();

    if (alreadyLoggedIn) {
        return "LOGIN_FAILED|该用户已在其他客户端登录";
    } else {
        usersMutex.lock();
        User *user = User::login(users, username, password);
        usersMutex.unlock();

        if (user) { // 添加到已登录用户集合
            loggedInUsersMutex.lock();
            loggedInUsers.insert(username);
            loggedInUsersMutex.unlock();

            // 记录此连接的用户名
            currentConnectedUser = username;

            return "LOGIN_SUCCESS|" + user->getUsername() + "|" + user->getUserType();
        } else {
            return "LOGIN_FAILED|用户名或密码错误";
        }
    }
}

// 处理注册请求
std::string Server::handleRegisterRequest(const std::string &request) {
    std::istringstream iss(request);
    std::string cmd, username, password, type;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, password, '|');
    std::getline(iss, type, '|');

    usersMutex.lock();
    bool exists = User::isUsernameExists(users, username);

    if (!exists) {
        User *newUser = nullptr;
        if (type == "customer") {
            newUser = new Customer(username, password, CART_FILE, 0.0);
        } else if (type == "seller") {
            newUser = new Seller(username, password, 0.0);
        }

        if (newUser) {
            users.push_back(newUser);
            User::saveUsersToFile(users, USER_FILE);
            usersMutex.unlock();
            return "REGISTER_SUCCESS|" + username;
        } else {
            usersMutex.unlock();
            return "REGISTER_FAILED|创建用户失败";
        }
    } else {
        usersMutex.unlock();
        return "REGISTER_FAILED|用户名已存在";
    }
}

// 处理获取商品列表请求
std::string Server::handleGetProductsRequest() const {
    storeMutex.lock();
    const auto &products = store.getProducts();
    std::ostringstream oss;
    oss << "PRODUCTS|" << products.size();

    for (const auto &product : products) {
        oss << "|" << product->getName()
            << "," << product->getType()
            << "," << product->getPrice()
            << "," << product->getQuantity()
            << "," << product->getSellerUsername();
    }
    storeMutex.unlock();

    return oss.str();
}

// 处理获取商品详情请求
std::string Server::handleGetProductDetailRequest(const std::string &request) const {
    std::istringstream iss(request);
    std::string cmd, productName, sellerUsername;

    std::getline(iss, cmd, '|');
    std::getline(iss, productName, '|');
    std::getline(iss, sellerUsername, '|');

    storeMutex.lock();
    Product *product = store.findProductByName(productName, sellerUsername);

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
            Book *book = dynamic_cast<Book *>(product);
            oss << "|" << book->getAuthor() << "|" << book->getIsbn();
        } else if (product->getType() == "Clothing") {
            Clothing *clothing = dynamic_cast<Clothing *>(product);
            oss << "|" << clothing->getSize() << "|" << clothing->getColor();
        } else if (product->getType() == "Food") {
            Food *food = dynamic_cast<Food *>(product);
            oss << "|" << food->getExpirationDate();
        }

        storeMutex.unlock();
        return oss.str();
    } else {
        storeMutex.unlock();
        return "PRODUCT_NOT_FOUND|产品不存在";
    }
}

// 处理添加到购物车请求
std::string Server::handleAddToCartRequest(const std::string &request) {
    std::istringstream iss(request);
    std::string cmd, username, productName, sellerUsername;
    int quantity;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, productName, '|');
    std::getline(iss, sellerUsername, '|');
    iss >> quantity;

    usersMutex.lock();
    storeMutex.lock();

    User *user = User::findUser(users, username);
    Customer *customer = dynamic_cast<Customer *>(user);
    Product *product = store.findProductByName(productName, sellerUsername);

    if (customer && product) {
        if (product->getQuantity() < quantity) {
            storeMutex.unlock();
            usersMutex.unlock();
            return "CART_ERROR|库存不足，当前库存: " + std::to_string(product->getQuantity());
        } else {
            // 直接操作 Customer 的 shoppingCartItems
            bool found = false;
            customer->loadCartFromFile(); // 确保购物车是最新的

            for (auto &item : customer->shoppingCartItems) {
                if (item.productName == productName && item.sellerUsername == sellerUsername) {
                    item.quantity += quantity;
                    found = true;
                    break;
                }
            }

            if (!found) {
                // 添加新商品到购物车
                CartItem newItem;
                newItem.productId = product->getName(); // 为了兼容性，保留 productId
                newItem.productName = product->getName();
                newItem.sellerUsername = sellerUsername;
                newItem.quantity = quantity;
                newItem.priceAtAddition = product->getPrice();
                customer->shoppingCartItems.push_back(newItem);
            }

            customer->saveCartToFile();
            storeMutex.unlock();
            usersMutex.unlock();
            return "CART_UPDATED|添加成功";
        }
    } else {
        storeMutex.unlock();
        usersMutex.unlock();
        return "CART_ERROR|用户或产品不存在";
    }
}

// 处理查看购物车请求
std::string Server::handleGetCartRequest(const std::string &request) const {
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    usersMutex.lock();
    User *user = User::findUser(users, username);
    Customer *customer = dynamic_cast<Customer *>(user);

    if (customer) {
        customer->loadCartFromFile();
        std::ostringstream oss;
        oss << "CART|" << customer->shoppingCartItems.size();

        for (const auto &item : customer->shoppingCartItems) {
            oss << "|" << item.productName
                << "," << item.quantity
                << "," << item.priceAtAddition
                << "," << item.sellerUsername;
        }

        usersMutex.unlock();
        return oss.str();
    } else {
        usersMutex.unlock();
        return "CART_ERROR|用户不存在或不是消费者";
    }
}

// 处理结算请求
std::string Server::handleCheckoutRequest(const std::string &request) {
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    usersMutex.lock();
    storeMutex.lock();

    User *user = User::findUser(users, username);
    Customer *customer = dynamic_cast<Customer *>(user);

    if (customer) {
        customer->loadCartFromFile();

        if (customer->isCartEmpty()) {
            storeMutex.unlock();
            usersMutex.unlock();
            return "CHECKOUT_ERROR|购物车为空";
        }

        Order order(username);
        double totalAmount = 0.0;
        bool allItemsValid = true;

        for (const auto &item : customer->shoppingCartItems) {
            Product *product = store.findProductByName(item.productName, item.sellerUsername);
            if (product && product->getQuantity() >= item.quantity) {
                order.addItemFromProduct(*product, item.quantity);
                totalAmount += item.quantity * product->getPrice();
            } else {
                allItemsValid = false;
                break;
            }
        }

        if (allItemsValid && customer->checkBalance() >= totalAmount) {
            customer->withdraw(totalAmount);
            order.calculateTotalAmount();
            auto orderPtr = orderManager.submitOrderRequest(order);

            // 等待订单处理完成
            auto startTime = std::chrono::steady_clock::now();
            bool orderProcessed = false;

            // 最多等待10秒
            while (!orderProcessed &&
                   std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::steady_clock::now() - startTime)
                           .count() < 10) {
                if (orderPtr->getProcessed()) {
                    orderProcessed = true;
                    break;
                }

                // 处理订单并更新库存（如果有订单在队列中）
                orderManager.processNextOrder(store, users);

                // 短暂暂停，避免CPU过度使用
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if (orderProcessed) {
                if (orderPtr->getStatus() == "COMPLETED" ||
                    orderPtr->getStatus() == "COMPLETED_WITH_PAYMENT_ISSUES") {
                    // 清空购物车
                    customer->clearCartAndFile();
                    
                    storeMutex.unlock();
                    usersMutex.unlock();
                    return "CHECKOUT_SUCCESS|订单已完成|" + orderPtr->getOrderId();
                } else {
                    // 订单处理失败
                    storeMutex.unlock();
                    usersMutex.unlock();
                    return "CHECKOUT_ERROR|订单处理失败: " + orderPtr->getStatus();
                }
            } else {
                // 订单处理超时
                storeMutex.unlock();
                usersMutex.unlock();
                return "CHECKOUT_PENDING|订单已提交但处理中|" + orderPtr->getOrderId();
            }
        } else if (!allItemsValid) {
            storeMutex.unlock();
            usersMutex.unlock();
            return "CHECKOUT_ERROR|部分商品库存不足";
        } else {
            storeMutex.unlock();
            usersMutex.unlock();
            return "CHECKOUT_ERROR|余额不足，需要 " + std::to_string(totalAmount) + " 元";
        }
    } else {
        storeMutex.unlock();
        usersMutex.unlock();
        return "CHECKOUT_ERROR|用户不存在或不是消费者";
    }
}

// 处理登出请求
std::string Server::handleLogoutRequest(const std::string &request, std::string &currentConnectedUser) {
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    loggedInUsersMutex.lock();
    auto it = loggedInUsers.find(username);
    if (it != loggedInUsers.end()) {
        loggedInUsers.erase(it);
        currentConnectedUser = ""; // 清除当前连接的用户名
        loggedInUsersMutex.unlock();
        return "LOGOUT_SUCCESS|用户已登出";
    } else {
        loggedInUsersMutex.unlock();
        return "LOGOUT_FAILED|用户未登录";
    }
}

// 处理获取用户信息请求
std::string Server::handleGetUserInfoRequest(const std::string &request) const {
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    usersMutex.lock();
    User *user = User::findUser(users, username);
    
    if (user) {
        std::ostringstream oss;
        oss << "USER_INFO|" 
            << user->getUsername() << "|" 
            << user->getUserType() << "|" 
            << user->checkBalance();
        
        usersMutex.unlock();
        return oss.str();
    } else {
        usersMutex.unlock();
        return "USER_INFO_ERROR|用户不存在";
    }
}

// 处理查询余额请求
std::string Server::handleCheckBalanceRequest(const std::string &request) const {
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    usersMutex.lock();
    User *user = User::findUser(users, username);
    
    if (user) {
        double balance = user->checkBalance();
        usersMutex.unlock();
        return "BALANCE|" + std::to_string(balance);
    } else {
        usersMutex.unlock();
        return "BALANCE_ERROR|用户不存在";
    }
}

// 处理充值请求
std::string Server::handleDepositRequest(const std::string &request) {
    std::istringstream iss(request);
    std::string cmd, username, amountStr;
    double amount;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, amountStr, '|');
    
    try {
        amount = std::stod(amountStr);
    } catch (const std::exception&) {
        return "BALANCE_ERROR|无效金额";
    }

    if (amount <= 0) {
        return "BALANCE_ERROR|金额必须大于零";
    }

    usersMutex.lock();
    User *user = User::findUser(users, username);
    
    if (user) {
        if (user->deposit(amount)) {
            double newBalance = user->checkBalance();
            User::saveUsersToFile(users, USER_FILE); // 保存更新后的余额
            usersMutex.unlock();
            return "BALANCE_UPDATED|充值成功|" + std::to_string(newBalance);
        } else {
            usersMutex.unlock();
            return "BALANCE_ERROR|充值失败";
        }
    } else {
        usersMutex.unlock();
        return "BALANCE_ERROR|用户不存在";
    }
}

// 主函数
int main() {
    Server server;
    
    if (!server.initialize()) {
        std::cerr << "服务器初始化失败" << std::endl;
        return 1;
    }
    
    if (!server.start()) {
        std::cerr << "服务器启动失败" << std::endl;
        return 1;
    }
    
    server.run();
    
    return 0;
}
