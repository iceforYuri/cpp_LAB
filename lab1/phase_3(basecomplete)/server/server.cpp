#include "server.h"

// 构造函数
Server::Server(const std::string &userFile, const std::string &storeFile,
               const std::string &cartFile, const std::string &orderDir)
    : serverSocket(INVALID_SOCKET),
      serverRunning(true),
      USER_FILE(userFile),
      STORE_FILE(storeFile),
      CART_FILE(cartFile),
      ORDER_DIR(orderDir),
      store(storeFile),
      orderManager(orderDir)
{
}

// 析构函数
Server::~Server()
{
    stop();

    // 释放资源
    for (auto *user : users)
    {
        delete user;
    }

    if (serverSocket != INVALID_SOCKET)
    {
        closesocket(serverSocket);
    }
    WSACleanup();
}

// 初始化服务器
bool Server::initialize()
{
    // 设置控制台输出编码为 UTF-8
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    // 初始化 Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup 失败: " << result << std::endl;
        return false;
    }

    // 创建服务器socket
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
    {
        std::cerr << "创建socket失败: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    // 绑定到本地地址和端口
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888); // 使用8888端口

    if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "绑定失败: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return false;
    }

    // 开始监听
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "监听失败: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return false;
    }

    std::cout << "服务器初始化完成。" << std::endl;

    // 加载用户数据
    users = User::loadUsersFromFile(USER_FILE);
    std::cout << "已加载 " << users.size() << " 个用户数据。" << std::endl;

    // 加载商品数据
    store.loadAllProducts();
    std::cout << "已加载商品数据。" << std::endl;

    return true;
}

// 启动服务器
void Server::start()
{
    std::cout << "服务器启动，等待连接..." << std::endl;

    // 启动订单处理线程
    orderManager.startProcessingThread(store, users);
    std::cout << "订单处理线程已启动。" << std::endl;

    // 主循环，接受客户端连接
    while (serverRunning)
    {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "接受连接失败: " << WSAGetLastError() << std::endl;
            continue;
        }

        // 为每个客户端创建新线程
        clientThreads.emplace_back(&Server::handleClient, this, clientSocket);
    }

    // 等待所有客户端线程结束
    for (auto &thread : clientThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    // 停止订单处理线程
    orderManager.stopProcessingThread();

    // 保存用户数据
    User::saveUsersToFile(users, USER_FILE);
    std::cout << "用户数据已保存。" << std::endl;
}

// 停止服务器
void Server::stop()
{
    serverRunning = false;

    // 如果服务器socket有效，关闭它以中断accept调用
    if (serverSocket != INVALID_SOCKET)
    {
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
}

// 获取服务器状态
bool Server::isRunning() const
{
    return serverRunning;
}

// 处理客户端请求的函数
void Server::handleClient(SOCKET clientSocket)
{
    char buffer[4096] = {0};
    std::string response;
    bool clientConnected = true;
    std::string currentConnectedUser = ""; // 跟踪当前连接的用户名

    std::cout << "客户端已连接，处理中..." << std::endl;

    while (clientConnected && serverRunning)
    {
        // 清空缓冲区
        memset(buffer, 0, sizeof(buffer));

        // 接收客户端请求
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0)
        {
            std::cout << "客户端断开连接或接收错误" << std::endl;
            break;
        }

        // 解析请求
        std::string request(buffer);
        std::cout << "收到请求: " << request << std::endl;

        // 处理请求的核心逻辑
        std::string command = request.substr(0, request.find("|"));
        response = "ERROR|未知命令";

        if (command == "LOGIN")
        {
            response = processLoginRequest(request);

            // 检查是否登录成功，如果是则更新当前连接用户
            if (response.substr(0, 13) == "LOGIN_SUCCESS")
            {
                std::istringstream iss(request);
                std::string cmd, username;
                std::getline(iss, cmd, '|');
                std::getline(iss, username, '|');
                currentConnectedUser = username;
            }
        }
        else if (command == "REGISTER")
        {
            response = processRegisterRequest(request);
        }
        else if (command == "GET_PRODUCTS")
        {
            response = processGetProductsRequest(request);
        }
        else if (command == "GET_PRODUCT_DETAIL")
        {
            response = processGetProductDetailRequest(request);
        }
        else if (command == "ADD_TO_CART")
        {
            response = processAddToCartRequest(request);
        }
        else if (command == "GET_CART")
        {
            response = processGetCartRequest(request);
        }
        else if (command == "CHECKOUT")
        {
            response = processCheckoutRequest(request);
        }
        else if (command == "GET_SELLER_INFO")
        {
            response = processGetSellerInfoRequest(request);
        }
        else if (command == "CHANGE_PASSWORD")
        {
            response = processChangePasswordRequest(request);
        }
        else if (command == "GET_SELLER_PRODUCTS")
        {
            response = processGetSellerProductsRequest(request);
        }
        else if (command == "ADD_BOOK")
        {
            response = processAddBookRequest(request);
        }
        else if (command == "ADD_CLOTHING")
        {
            response = processAddClothingRequest(request);
        }
        else if (command == "ADD_FOOD")
        {
            response = processAddFoodRequest(request);
        }
        else if (command == "ADD_GENERIC")
        {
            response = processAddGenericRequest(request);
        }
        else if (command == "UPDATE_PRODUCT_PRICE")
        {
            response = processUpdateProductPriceRequest(request);
        }
        else if (command == "UPDATE_PRODUCT_QUANTITY")
        {
            response = processUpdateProductQuantityRequest(request);
        }
        else if (command == "SET_PRODUCT_DISCOUNT")
        {
            response = processSetProductDiscountRequest(request);
        }
        else if (command == "SET_CATEGORY_DISCOUNT")
        {
            response = processSetCategoryDiscountRequest(request);
        }
        else if (command == "LOGOUT")
        {
            response = processLogoutRequest(request);

            // 检查是否登出成功，如果是则清除当前连接用户
            if (response.substr(0, 14) == "LOGOUT_SUCCESS" &&
                request.find(currentConnectedUser) != std::string::npos)
            {
                currentConnectedUser = "";
            }
        }
        else if (command == "CHECK_BALANCE")
        {
            response = processCheckBalanceRequest(request);
        }
        else if (command == "GET_USER_INFO")
        {
            response = processGetUserInfoRequest(request);
        }
        else if (command == "DEPOSIT")
        {
            response = processDepositRequest(request);
        }
        else if (command == "SEARCH_PRODUCTS")
        {
            response = processSearchProductsRequest(request);
        }
        else if (command == "DIRECT_PURCHASE")
        {
            response = processDirectPurchaseRequest(request);
        }
        else if (command == "UPDATE_CART_ITEM")
        {
            response = processUpdateCartItemRequest(request);
        }
        else if (command == "REMOVE_FROM_CART")
        {
            response = processRemoveFromCartRequest(request);
        }
        std::cout << "处理结果: " << response << std::endl;
        // 发送响应给客户端
        send(clientSocket, response.c_str(), response.size(), 0);
    }

    // 检查此连接是否有关联的用户，如果有则从已登录用户集合中移除
    if (!currentConnectedUser.empty())
    {
        loggedInUsersMutex.lock();
        auto it = loggedInUsers.find(currentConnectedUser);
        if (it != loggedInUsers.end())
        {
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
std::string Server::processLoginRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, password;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, password, '|');

    // 检查用户是否已经登录
    loggedInUsersMutex.lock();
    bool alreadyLoggedIn = (loggedInUsers.find(username) != loggedInUsers.end());
    loggedInUsersMutex.unlock();

    if (alreadyLoggedIn)
    {
        return "LOGIN_FAILED|该用户已在其他客户端登录";
    }
    else
    {
        usersMutex.lock();
        User *user = User::login(users, username, password);
        usersMutex.unlock();

        if (user)
        {
            // 添加到已登录用户集合
            loggedInUsersMutex.lock();
            loggedInUsers.insert(username);
            loggedInUsersMutex.unlock();

            return "LOGIN_SUCCESS|" + user->getUsername() + "|" + user->getUserType();
        }
        else
        {
            return "LOGIN_FAILED|用户名或密码错误";
        }
    }
}

// 处理注册请求
std::string Server::processRegisterRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, password, type;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, password, '|');
    std::getline(iss, type, '|');

    usersMutex.lock();
    bool exists = User::isUsernameExists(users, username);

    if (!exists)
    {
        User *newUser = nullptr;
        if (type == "customer")
        {
            newUser = new Customer(username, password, CART_FILE, 0.0);
        }
        else if (type == "seller")
        {
            newUser = new Seller(username, password, 0.0);
        }

        if (newUser)
        {
            users.push_back(newUser);
            User::saveUsersToFile(users, USER_FILE);
            usersMutex.unlock();
            return "REGISTER_SUCCESS|" + username;
        }
        else
        {
            usersMutex.unlock();
            return "REGISTER_FAILED|创建用户失败";
        }
    }
    else
    {
        usersMutex.unlock();
        return "REGISTER_FAILED|用户名已存在";
    }
}

// 处理获取商品列表请求
std::string Server::processGetProductsRequest(const std::string &request) const
{
    storeMutex.lock();
    const auto &products = store.getProducts();
    std::ostringstream oss;
    oss << "PRODUCTS|" << products.size();

    for (const auto &product : products)
    {
        oss << "|" << product->getName()
            << "," << product->getType()
            << "," << product->getoriginalPrice()
            << "," << product->getPrice()
            << "," << product->getQuantity()
            << "," << product->getSellerUsername();
    }
    storeMutex.unlock();

    return oss.str();
}

// 处理获取商品详情请求
std::string Server::processGetProductDetailRequest(const std::string &request) const
{
    std::istringstream iss(request);
    std::string cmd, productName, sellerUsername;

    std::getline(iss, cmd, '|');
    std::getline(iss, productName, '|');
    std::getline(iss, sellerUsername, '|');

    storeMutex.lock();
    Product *product = store.findProductByName(productName, sellerUsername);

    if (product)
    {
        std::ostringstream oss;
        oss << "PRODUCT_DETAIL|" << product->getName()
            << "|" << product->getType()
            << "|" << product->getDescription()
            << "|" << product->getPrice()
            << "|" << product->getQuantity()
            << "|" << product->getSellerUsername();

        // 根据不同类型添加特定属性
        if (product->getType() == "Book")
        {
            Book *book = dynamic_cast<Book *>(product);
            oss << "|" << book->getAuthor() << "|" << book->getIsbn();
        }
        else if (product->getType() == "Clothing")
        {
            Clothing *clothing = dynamic_cast<Clothing *>(product);
            oss << "|" << clothing->getSize() << "|" << clothing->getColor();
        }
        else if (product->getType() == "Food")
        {
            Food *food = dynamic_cast<Food *>(product);
            oss << "|" << food->getExpirationDate();
        }

        storeMutex.unlock();
        return oss.str();
    }
    else
    {
        storeMutex.unlock();
        return "PRODUCT_NOT_FOUND|产品不存在";
    }
}

// 处理添加到购物车请求
std::string Server::processAddToCartRequest(const std::string &request)
{
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

    if (customer && product)
    {
        if (product->getQuantity() < quantity)
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "CART_ERROR|库存不足，当前库存: " + std::to_string(product->getQuantity());
        }
        else
        {
            // 直接操作 Customer 的 shoppingCartItems
            bool found = false;
            customer->loadCartFromFile(); // 确保购物车是最新的

            for (auto &item : customer->shoppingCartItems)
            {
                if (item.productName == productName && item.sellerUsername == sellerUsername)
                {
                    item.quantity += quantity;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
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
    }
    else
    {
        storeMutex.unlock();
        usersMutex.unlock();
        return "CART_ERROR|用户或产品不存在";
    }
}

// 其他方法的实现将在后续补充...

// 处理获取购物车请求
std::string Server::processGetCartRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    usersMutex.lock();
    User *user = User::findUser(users, username);
    Customer *customer = dynamic_cast<Customer *>(user);

    if (customer)
    {
        customer->loadCartFromFile();
        std::ostringstream oss;
        oss << "CART|" << customer->shoppingCartItems.size();

        for (const auto &item : customer->shoppingCartItems)
        {
            oss << "|" << item.productName
                << "," << item.quantity
                << "," << item.priceAtAddition
                << "," << item.sellerUsername;
        }

        usersMutex.unlock();
        return oss.str();
    }
    else
    {
        usersMutex.unlock();
        return "CART_ERROR|用户不存在或不是消费者";
    }
}

// 处理结算购物车请求
std::string Server::processCheckoutRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    usersMutex.lock();
    storeMutex.lock();

    User *user = User::findUser(users, username);
    Customer *customer = dynamic_cast<Customer *>(user);

    if (customer)
    {
        customer->loadCartFromFile();

        if (customer->isCartEmpty())
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "CHECKOUT_ERROR|购物车为空";
        }
        else
        {
            Order order(username);
            double totalAmount = 0.0;
            bool allItemsValid = true;

            for (const auto &item : customer->shoppingCartItems)
            {
                Product *product = store.findProductByName(item.productName, item.sellerUsername);
                if (product && product->getQuantity() >= item.quantity)
                {
                    order.addItemFromProduct(*product, item.quantity);
                    totalAmount += item.quantity * product->getPrice();
                }
                else
                {
                    allItemsValid = false;
                    break;
                }
            }

            if (allItemsValid && customer->checkBalance() >= totalAmount)
            {
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
                               .count() < 10)
                {
                    if (orderPtr->getProcessed())
                    {
                        orderProcessed = true;
                        break;
                    }

                    // 处理订单并更新库存（如果有订单在队列中）
                    orderManager.processNextOrder(store, users);

                    // 短暂暂停，避免CPU过度使用
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                if (orderProcessed)
                {
                    if (orderPtr->getStatus() == "COMPLETED" ||
                        orderPtr->getStatus() == "COMPLETED_WITH_PAYMENT_ISSUES")
                    {
                        // 清空购物车
                        customer->clearCartAndFile();

                        // 保存用户数据
                        User::saveUsersToFile(users, USER_FILE);

                        storeMutex.unlock();
                        usersMutex.unlock();
                        return "CHECKOUT_SUCCESS|订单已提交|" + orderPtr->getOrderId();
                    }
                    else
                    {
                        // 订单处理失败，返回错误信息
                        storeMutex.unlock();
                        usersMutex.unlock();
                        return "CHECKOUT_ERROR|订单处理失败: " + orderPtr->getStatus();
                    }
                }
                else
                {
                    // 订单处理超时，返回等待信息
                    storeMutex.unlock();
                    usersMutex.unlock();
                    return "CHECKOUT_PENDING|订单正在处理中，请稍后查询|" + orderPtr->getOrderId();
                }
            }
            else if (!allItemsValid)
            {
                storeMutex.unlock();
                usersMutex.unlock();
                return "CHECKOUT_ERROR|部分商品库存不足";
            }
            else
            {
                storeMutex.unlock();
                usersMutex.unlock();
                return "CHECKOUT_ERROR|余额不足";
            }
        }
    }
    else
    {
        storeMutex.unlock();
        usersMutex.unlock();
        return "CHECKOUT_ERROR|用户不存在或不是消费者";
    }
}

// 处理获取商家信息请求
std::string Server::processGetSellerInfoRequest(const std::string &request) const
{
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    usersMutex.lock();
    User *user = User::findUser(users, username);

    if (user && user->getUserType() == "seller")
    {
        std::ostringstream oss;
        oss << "SELLER_INFO|" << user->getUsername() << "|"
            << user->getUserType() << "|" << user->checkBalance();

        usersMutex.unlock();
        return oss.str();
    }
    else
    {
        usersMutex.unlock();
        return "SELLER_ERROR|用户不存在或不是商家";
    }
}

// 处理修改密码请求
std::string Server::processChangePasswordRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, oldPassword, newPassword;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, oldPassword, '|');
    std::getline(iss, newPassword, '|');

    usersMutex.lock();
    User *user = User::findUser(users, username);

    if (user)
    {
        if (user->changePassword(oldPassword, newPassword))
        {
            User::saveUsersToFile(users, USER_FILE);
            usersMutex.unlock();
            return "PASSWORD_CHANGED|密码修改成功";
        }
        else
        {
            usersMutex.unlock();
            return "PASSWORD_ERROR|密码修改失败，可能原密码错误";
        }
    }
    else
    {
        usersMutex.unlock();
        return "PASSWORD_ERROR|用户不存在";
    }
}

// 处理获取商家商品请求
std::string Server::processGetSellerProductsRequest(const std::string &request) const
{
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    storeMutex.lock();
    std::vector<Product *> sellerProducts = store.getSellerProducts(username);

    std::ostringstream oss;
    oss << "SELLER_PRODUCTS|" << sellerProducts.size();

    for (const auto &product : sellerProducts)
    {
        oss << "|" << product->getName()
            << "," << product->getType()
            << "," << product->getOriginalPrice()
            << "," << product->getQuantity()
            << "," << product->getDiscountRate();
    }
    storeMutex.unlock();

    return oss.str();
}

// 处理添加书籍请求
std::string Server::processAddBookRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, name, description, priceStr, quantityStr, author, isbn;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, name, '|');
    std::getline(iss, description, '|');
    std::getline(iss, priceStr, '|');
    std::getline(iss, quantityStr, '|');
    std::getline(iss, author, '|');
    std::getline(iss, isbn, '|');

    double price = std::stod(priceStr);
    int quantity = std::stoi(quantityStr);

    usersMutex.lock();
    storeMutex.lock();

    User *user = User::findUser(users, username);

    if (user && user->getUserType() == "seller")
    {
        if (store.createBook(user, name, description, price, quantity, author, isbn))
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PRODUCT_ADDED|书籍添加成功";
        }
        else
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PRODUCT_ERROR|书籍添加失败，可能是名称重复";
        }
    }
    else
    {
        storeMutex.unlock();
        usersMutex.unlock();
        return "PRODUCT_ERROR|用户不存在或不是商家";
    }
}

// 处理添加服装请求
std::string Server::processAddClothingRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, name, description, priceStr, quantityStr, size, color;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, name, '|');
    std::getline(iss, description, '|');
    std::getline(iss, priceStr, '|');
    std::getline(iss, quantityStr, '|');
    std::getline(iss, size, '|');
    std::getline(iss, color, '|');

    double price = std::stod(priceStr);
    int quantity = std::stoi(quantityStr);

    usersMutex.lock();
    storeMutex.lock();

    User *user = User::findUser(users, username);

    if (user && user->getUserType() == "seller")
    {
        if (store.createClothing(user, name, description, price, quantity, size, color))
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PRODUCT_ADDED|服装添加成功";
        }
        else
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PRODUCT_ERROR|服装添加失败，可能是名称重复";
        }
    }
    else
    {
        storeMutex.unlock();
        usersMutex.unlock();
        return "PRODUCT_ERROR|用户不存在或不是商家";
    }
}

// 处理添加食品请求
std::string Server::processAddFoodRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, name, description, priceStr, quantityStr, expDate;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, name, '|');
    std::getline(iss, description, '|');
    std::getline(iss, priceStr, '|');
    std::getline(iss, quantityStr, '|');
    std::getline(iss, expDate, '|');

    double price = std::stod(priceStr);
    int quantity = std::stoi(quantityStr);

    usersMutex.lock();
    storeMutex.lock();

    User *user = User::findUser(users, username);

    if (user && user->getUserType() == "seller")
    {
        if (store.createFood(user, name, description, price, quantity, expDate))
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PRODUCT_ADDED|食品添加成功";
        }
        else
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PRODUCT_ERROR|食品添加失败，可能是名称重复";
        }
    }
    else
    {
        storeMutex.unlock();
        usersMutex.unlock();
        return "PRODUCT_ERROR|用户不存在或不是商家";
    }
}

// 处理添加通用商品请求
std::string Server::processAddGenericRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, name, description, priceStr, quantityStr, categoryTag;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, name, '|');
    std::getline(iss, description, '|');
    std::getline(iss, priceStr, '|');
    std::getline(iss, quantityStr, '|');
    std::getline(iss, categoryTag, '|');

    double price = std::stod(priceStr);
    int quantity = std::stoi(quantityStr);

    usersMutex.lock();
    storeMutex.lock();

    User *user = User::findUser(users, username);

    if (user && user->getUserType() == "seller")
    {
        if (store.createGenericProduct(user, name, description, price, quantity, categoryTag))
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PRODUCT_ADDED|通用商品添加成功";
        }
        else
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PRODUCT_ERROR|通用商品添加失败，可能是名称重复";
        }
    }
    else
    {
        storeMutex.unlock();
        usersMutex.unlock();
        return "PRODUCT_ERROR|用户不存在或不是商家";
    }
}

// 处理更新商品价格请求
std::string Server::processUpdateProductPriceRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, productName, priceStr;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, productName, '|');
    std::getline(iss, priceStr, '|');

    double newPrice = std::stod(priceStr);

    usersMutex.lock();
    storeMutex.lock();

    User *user = User::findUser(users, username);

    if (user && user->getUserType() == "seller")
    {
        if (store.manageProductPrice(user, productName, newPrice))
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PRODUCT_UPDATED|价格修改成功";
        }
        else
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PRODUCT_ERROR|价格修改失败，可能商品不存在或不属于该商家";
        }
    }
    else
    {
        storeMutex.unlock();
        usersMutex.unlock();
        return "PRODUCT_ERROR|用户不存在或不是商家";
    }
}

// 处理更新商品库存请求
std::string Server::processUpdateProductQuantityRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, productName, quantityStr;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, productName, '|');
    std::getline(iss, quantityStr, '|');

    int newQuantity = std::stoi(quantityStr);

    usersMutex.lock();
    storeMutex.lock();

    User *user = User::findUser(users, username);

    if (user && user->getUserType() == "seller")
    {
        if (store.manageProductQuantity(user, productName, newQuantity))
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PRODUCT_UPDATED|库存修改成功";
        }
        else
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PRODUCT_ERROR|库存修改失败，可能商品不存在或不属于该商家";
        }
    }
    else
    {
        storeMutex.unlock();
        usersMutex.unlock();
        return "PRODUCT_ERROR|用户不存在或不是商家";
    }
}

// 处理设置商品折扣请求
std::string Server::processSetProductDiscountRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, productName, discountStr;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, productName, '|');
    std::getline(iss, discountStr, '|');

    double discount = std::stod(discountStr);

    usersMutex.lock();
    storeMutex.lock();

    User *user = User::findUser(users, username);

    if (user && user->getUserType() == "seller")
    {
        if (store.manageProductDiscount(user, productName, discount))
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PRODUCT_UPDATED|折扣设置成功";
        }
        else
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PRODUCT_ERROR|折扣设置失败，可能商品不存在或不属于该商家";
        }
    }
    else
    {
        storeMutex.unlock();
        usersMutex.unlock();
        return "PRODUCT_ERROR|用户不存在或不是商家";
    }
}

// 处理设置类别折扣请求
std::string Server::processSetCategoryDiscountRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, category, discountStr;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, category, '|');
    std::getline(iss, discountStr, '|');

    double discount = std::stod(discountStr);

    usersMutex.lock();
    storeMutex.lock();

    User *user = User::findUser(users, username);

    if (user && user->getUserType() == "seller")
    {
        if (store.applyCategoryDiscount(user, category, discount))
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PRODUCT_UPDATED|类别折扣设置成功";
        }
        else
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PRODUCT_ERROR|类别折扣设置失败，可能该类别不存在或没有商品";
        }
    }
    else
    {
        storeMutex.unlock();
        usersMutex.unlock();
        return "PRODUCT_ERROR|用户不存在或不是商家";
    }
}

// 处理登出请求
std::string Server::processLogoutRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    loggedInUsersMutex.lock();
    auto it = loggedInUsers.find(username);
    if (it != loggedInUsers.end())
    {
        loggedInUsers.erase(it);
        loggedInUsersMutex.unlock();
        return "LOGOUT_SUCCESS|用户已成功登出";
    }
    else
    {
        loggedInUsersMutex.unlock();
        return "LOGOUT_FAILED|用户未登录";
    }
}

// 处理检查余额请求
std::string Server::processCheckBalanceRequest(const std::string &request) const
{
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    usersMutex.lock();
    User *user = User::findUser(users, username);

    if (user)
    {
        double balance = user->checkBalance();
        usersMutex.unlock();
        return "BALANCE|" + std::to_string(balance);
    }
    else
    {
        usersMutex.unlock();
        return "BALANCE_ERROR|用户不存在";
    }
}

// 处理获取用户信息请求
std::string Server::processGetUserInfoRequest(const std::string &request) const
{
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    usersMutex.lock();
    User *user = User::findUser(users, username);

    if (user)
    {
        std::ostringstream oss;
        oss << "USER_INFO|" << user->getUsername() << "|"
            << user->getUserType() << "|" << user->checkBalance();

        usersMutex.unlock();
        return oss.str();
    }
    else
    {
        usersMutex.unlock();
        return "USER_ERROR|用户不存在";
    }
}

// 处理充值请求
std::string Server::processDepositRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, amountStr;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, amountStr, '|');

    double amount = std::stod(amountStr);

    usersMutex.lock();
    User *user = User::findUser(users, username);

    if (user)
    {
        if (amount <= 0)
        {
            usersMutex.unlock();
            return "BALANCE_ERROR|充值金额必须大于0";
        }

        if (user->deposit(amount))
        {
            User::saveUsersToFile(users, USER_FILE);
            double newBalance = user->checkBalance();
            usersMutex.unlock();
            return "BALANCE_UPDATED|充值成功|" + std::to_string(newBalance);
        }
        else
        {
            usersMutex.unlock();
            return "BALANCE_ERROR|充值失败";
        }
    }
    else
    {
        usersMutex.unlock();
        return "BALANCE_ERROR|用户不存在";
    }
}

// 处理搜索商品请求
std::string Server::processSearchProductsRequest(const std::string &request) const
{
    std::istringstream iss(request);
    std::string cmd, searchTerm, sellerUsername;

    std::getline(iss, cmd, '|');
    std::getline(iss, searchTerm, '|');
    std::getline(iss, sellerUsername, '|');

    storeMutex.lock();
    std::vector<Product *> searchResults = store.searchProductsByName(searchTerm, sellerUsername);

    std::ostringstream oss;
    oss << "SEARCH_RESULTS|" << searchResults.size();

    for (const auto &product : searchResults)
    {
        oss << "|" << product->getName()
            << "," << product->getType()
            << "," << product->getPrice()
            << "," << product->getQuantity()
            << "," << product->getSellerUsername();
    }
    storeMutex.unlock();

    return oss.str();
}

// 处理直接购买请求
std::string Server::processDirectPurchaseRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, productName, sellerUsername, quantityStr;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, productName, '|');
    std::getline(iss, sellerUsername, '|');
    std::getline(iss, quantityStr, '|');

    int quantity = std::stoi(quantityStr);

    usersMutex.lock();
    storeMutex.lock();

    User *user = User::findUser(users, username);
    Customer *customer = dynamic_cast<Customer *>(user);
    Product *product = store.findProductByName(productName, sellerUsername);

    if (customer && product)
    {
        if (product->getQuantity() < quantity)
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PURCHASE_ERROR|库存不足，当前库存: " + std::to_string(product->getQuantity());
        }

        double totalCost = product->getPrice() * quantity;
        if (customer->checkBalance() < totalCost)
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "PURCHASE_ERROR|余额不足，需要: " + std::to_string(totalCost) + ", 当前余额: " + std::to_string(customer->checkBalance());
        }

        // 创建订单
        Order order(username);
        order.addItemFromProduct(*product, quantity);
        order.calculateTotalAmount();

        // 扣款
        customer->withdraw(totalCost);

        // 提交订单
        auto orderPtr = orderManager.submitOrderRequest(order);

        // 等待订单处理完成
        auto startTime = std::chrono::steady_clock::now();
        bool orderProcessed = false;

        // 最多等待10秒
        while (!orderProcessed &&
               std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::steady_clock::now() - startTime)
                       .count() < 10)
        {
            if (orderPtr->getProcessed())
            {
                orderProcessed = true;
                break;
            }

            // 处理订单并更新库存
            orderManager.processNextOrder(store, users);

            // 短暂暂停，避免CPU过度使用
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (orderProcessed)
        {
            if (orderPtr->getStatus() == "COMPLETED" ||
                orderPtr->getStatus() == "COMPLETED_WITH_PAYMENT_ISSUES")
            {

                // 保存用户数据
                User::saveUsersToFile(users, USER_FILE);

                storeMutex.unlock();
                usersMutex.unlock();
                return "PURCHASE_SUCCESS|购买成功|" + orderPtr->getOrderId();
            }
            else
            {
                // 订单处理失败，返回错误信息
                storeMutex.unlock();
                usersMutex.unlock();
                return "PURCHASE_ERROR|订单处理失败: " + orderPtr->getStatus();
            }
        }
        else
        {
            // 订单处理超时，返回等待信息
            storeMutex.unlock();
            usersMutex.unlock();
            return "PURCHASE_PENDING|订单正在处理中，请稍后查询|" + orderPtr->getOrderId();
        }
    }
    else
    {
        storeMutex.unlock();
        usersMutex.unlock();
        return "PURCHASE_ERROR|用户或商品不存在";
    }
}

// 处理更新购物车商品数量请求
std::string Server::processUpdateCartItemRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, productName, sellerUsername, quantityStr;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, productName, '|');
    std::getline(iss, sellerUsername, '|');
    std::getline(iss, quantityStr, '|');

    int newQuantity = std::stoi(quantityStr);

    usersMutex.lock();
    storeMutex.lock();

    User *user = User::findUser(users, username);
    Customer *customer = dynamic_cast<Customer *>(user);

    if (customer)
    {
        customer->loadCartFromFile();
        bool updated = false;

        for (auto &item : customer->shoppingCartItems)
        {
            if (item.productName == productName && item.sellerUsername == sellerUsername)
            {
                // 检查新数量是否有效
                Product *product = store.findProductByName(productName, sellerUsername);
                if (product && product->getQuantity() >= newQuantity)
                {
                    item.quantity = newQuantity;
                    updated = true;
                }
                else if (product)
                {
                    storeMutex.unlock();
                    usersMutex.unlock();
                    return "CART_ERROR|库存不足，当前库存: " + std::to_string(product->getQuantity());
                }
                break;
            }
        }

        if (updated)
        {
            customer->saveCartToFile();
            storeMutex.unlock();
            usersMutex.unlock();
            return "CART_UPDATED|商品数量已更新";
        }
        else
        {
            storeMutex.unlock();
            usersMutex.unlock();
            return "CART_ERROR|购物车中未找到该商品";
        }
    }
    else
    {
        storeMutex.unlock();
        usersMutex.unlock();
        return "CART_ERROR|用户不存在或不是消费者";
    }
}

// 处理从购物车移除商品请求
std::string Server::processRemoveFromCartRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, productName, sellerUsername;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, productName, '|');
    std::getline(iss, sellerUsername, '|');

    usersMutex.lock();
    User *user = User::findUser(users, username);
    Customer *customer = dynamic_cast<Customer *>(user);

    if (customer)
    {
        bool removed = false;
        customer->loadCartFromFile();

        auto it = std::remove_if(customer->shoppingCartItems.begin(), customer->shoppingCartItems.end(),
                                 [&productName, &sellerUsername](const CartItem &item)
                                 {
                                     return (item.productName == productName && item.sellerUsername == sellerUsername);
                                 });

        if (it != customer->shoppingCartItems.end())
        {
            customer->shoppingCartItems.erase(it, customer->shoppingCartItems.end());
            customer->saveCartToFile();
            usersMutex.unlock();
            return "CART_UPDATED|商品已从购物车移除";
            removed = true;
        }

        if (!removed)
        {
            usersMutex.unlock();
            return "CART_ERROR|购物车中未找到该商品";
        }
    }
    else
    {
        usersMutex.unlock();
        return "CART_ERROR|用户不存在或不是消费者";
    }

    usersMutex.unlock();
    return "CART_ERROR|未知错误";
}
