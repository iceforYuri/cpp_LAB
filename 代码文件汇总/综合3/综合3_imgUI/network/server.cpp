#include "server.h"
#include "../user/user.h"
#include "../store/store.h"
#include "../order/ordermanager.h"
#include <iostream>
#include <sstream>
#include <random>
#include <algorithm>
#include <chrono>

// ClientSession 实现
ClientSession::ClientSession(SOCKET socket, const std::string &sid)
    : clientSocket(socket), sessionId(sid), userType(Protocol::UserType::CUSTOMER), isActive(true)
{
}

ClientSession::~ClientSession()
{
    stopSession();
}

void ClientSession::startSession(NetworkServer *server)
{
    clientThread = std::thread(&ClientSession::sessionLoop, this, server);
}

void ClientSession::stopSession()
{
    isActive.store(false);
    if (clientSocket != INVALID_SOCKET)
    {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
    if (clientThread.joinable())
    {
        clientThread.join();
    }
}

void ClientSession::sessionLoop(NetworkServer *server)
{
    std::cout << "客户端会话开始: " << sessionId << std::endl;

    while (isActive.load())
    {
        std::string rawData = receiveRawData();
        if (rawData.empty())
        {
            break;
        }

        Protocol::Message message = Protocol::Message::deserialize(rawData);
        message.sessionId = sessionId; // 确保会话ID正确

        // 处理消息
        server->handleClientMessage(shared_from_this(), message);
    }

    std::cout << "客户端会话结束: " << sessionId << std::endl;
}

bool ClientSession::sendRawData(const std::string &data)
{
    if (clientSocket == INVALID_SOCKET)
    {
        return false;
    }

    // 发送数据长度
    uint32_t dataLength = htonl(static_cast<uint32_t>(data.length()));
    if (send(clientSocket, reinterpret_cast<const char *>(&dataLength), sizeof(dataLength), 0) == SOCKET_ERROR)
    {
        return false;
    }

    // 发送数据内容
    int totalSent = 0;
    int dataSize = static_cast<int>(data.length());

    while (totalSent < dataSize)
    {
        int sent = send(clientSocket, data.c_str() + totalSent, dataSize - totalSent, 0);
        if (sent == SOCKET_ERROR)
        {
            return false;
        }
        totalSent += sent;
    }

    return true;
}

std::string ClientSession::receiveRawData()
{
    if (clientSocket == INVALID_SOCKET)
    {
        return "";
    }

    // 接收数据长度
    uint32_t dataLength;
    int received = recv(clientSocket, reinterpret_cast<char *>(&dataLength), sizeof(dataLength), 0);
    if (received <= 0)
    {
        isActive.store(false);
        return "";
    }

    dataLength = ntohl(dataLength);

    // 接收数据内容
    std::string data;
    data.resize(dataLength);

    int totalReceived = 0;
    while (totalReceived < static_cast<int>(dataLength))
    {
        int received = recv(clientSocket, &data[totalReceived], dataLength - totalReceived, 0);
        if (received <= 0)
        {
            isActive.store(false);
            return "";
        }
        totalReceived += received;
    }

    return data;
}

bool ClientSession::sendMessage(const Protocol::Message &message)
{
    std::string serialized = message.serialize();
    return sendRawData(serialized);
}

// NetworkServer 实现
NetworkServer::NetworkServer(int port)
    : port(port), serverSocket(INVALID_SOCKET), isRunning(false),
      userFile("./server_data/users.txt"),
      storeDir("./server_data/store"),
      orderDir("./server_data/orders")
{

    // 初始化Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup failed: " << result << std::endl;
    }
}

NetworkServer::~NetworkServer()
{
    stop();
    WSACleanup();
}

bool NetworkServer::start()
{
    if (isRunning.load())
    {
        return true;
    }

    // 创建socket
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        return false;
    }

    // 设置socket选项
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&opt), sizeof(opt));

    // 绑定地址
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        return false;
    }

    // 开始监听
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        return false;
    }

    // 初始化业务组件
    loadUserData();
    initializeStore();
    initializeOrderManager();

    isRunning.store(true);

    // 启动接受连接的线程
    acceptThread = std::thread(&NetworkServer::acceptLoop, this);

    return true;
}

void NetworkServer::stop()
{
    if (!isRunning.load())
    {
        return;
    }

    isRunning.store(false);

    // 关闭服务器socket
    if (serverSocket != INVALID_SOCKET)
    {
        closesocket(serverSocket);
        serverSocket = INVALID_SOCKET;
    }

    // 等待接受线程结束
    if (acceptThread.joinable())
    {
        acceptThread.join();
    }

    // 停止所有客户端会话
    {
        std::lock_guard<std::mutex> lock(sessionsMutex);
        for (auto &session : sessions)
        {
            session->stopSession();
        }
        sessions.clear();
    }

    // 保存数据
    saveUserData();

    std::cout << "服务器已停止" << std::endl;
}

void NetworkServer::acceptLoop()
{
    while (isRunning.load())
    {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET)
        {
            if (isRunning.load())
            {
                std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            }
            break;
        }

        // 生成会话ID
        std::string sessionId = generateSessionId();

        // 创建客户端会话
        auto session = std::make_shared<ClientSession>(clientSocket, sessionId);

        {
            std::lock_guard<std::mutex> lock(sessionsMutex);
            sessions.push_back(session);
        }

        // 启动会话
        session->startSession(this);

        std::cout << "新客户端连接，会话ID: " << sessionId << std::endl;
    }
}

std::string NetworkServer::generateSessionId()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(100000, 999999);

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    return "SESSION_" + std::to_string(timestamp) + "_" + std::to_string(dis(gen));
}

std::shared_ptr<ClientSession> NetworkServer::findSession(const std::string &sessionId)
{
    std::lock_guard<std::mutex> lock(sessionsMutex);
    auto it = std::find_if(sessions.begin(), sessions.end(),
                           [&sessionId](const std::shared_ptr<ClientSession> &session)
                           {
                               return session->getSessionId() == sessionId;
                           });

    return (it != sessions.end()) ? *it : nullptr;
}

void NetworkServer::removeSession(const std::string &sessionId)
{
    std::lock_guard<std::mutex> lock(sessionsMutex);
    sessions.erase(
        std::remove_if(sessions.begin(), sessions.end(),
                       [&sessionId](const std::shared_ptr<ClientSession> &session)
                       {
                           return session->getSessionId() == sessionId || !session->isSessionActive();
                       }),
        sessions.end());
}

// 添加查找指定用户名会话的方法
std::shared_ptr<ClientSession> NetworkServer::findSessionByUsername(const std::string &username)
{
    std::lock_guard<std::mutex> lock(sessionsMutex);
    auto it = std::find_if(sessions.begin(), sessions.end(),
                           [&username](const std::shared_ptr<ClientSession> &session)
                           {
                               return session->getUsername() == username && session->isSessionActive();
                           });

    return (it != sessions.end()) ? *it : nullptr;
}

// 处理客户端消息
void NetworkServer::handleClientMessage(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    switch (message.type)
    {
    case Protocol::MessageType::USER_LOGIN:
        handleUserLogin(session, message);
        break;
    case Protocol::MessageType::USER_REGISTER:
        handleUserRegister(session, message);
        break;
    case Protocol::MessageType::USER_LOGOUT:
        handleUserLogout(session, message);
        break;
    case Protocol::MessageType::USER_GET_INFO:
        handleUserGetInfo(session, message);
        break;
    case Protocol::MessageType::USER_UPDATE_BALANCE:
        handleUserUpdateBalance(session, message);
        break;
    case Protocol::MessageType::USER_CHANGE_PASSWORD:
        handleUserChangePassword(session, message);
        break;
    case Protocol::MessageType::PRODUCT_GET_ALL:
        handleProductGetAll(session, message);
        break;
    case Protocol::MessageType::PRODUCT_SEARCH:
        handleProductSearch(session, message);
        break;
    case Protocol::MessageType::CART_GET:
        handleCartGet(session, message);
        break;
    case Protocol::MessageType::CART_ADD_ITEM:
        handleCartAddItem(session, message);
        break;
    case Protocol::MessageType::CART_REMOVE_ITEM:
        handleCartRemoveItem(session, message);
        break;
    case Protocol::MessageType::CART_UPDATE_ITEM:
        handleCartUpdateItem(session, message);
        break;
    case Protocol::MessageType::CART_CLEAR:
        handleCartClear(session, message);
        break;
    case Protocol::MessageType::PRODUCT_ADD:
        handleProductAdd(session, message);
        break;
    case Protocol::MessageType::PRODUCT_MANAGE_PRICE:
        handleProductManagePrice(session, message);
        break;
    case Protocol::MessageType::PRODUCT_MANAGE_QUANTITY:
        handleProductManageQuantity(session, message);
        break;
    case Protocol::MessageType::PRODUCT_MANAGE_DISCOUNT:
        handleProductManageDiscount(session, message);
        break;
    case Protocol::MessageType::PRODUCT_APPLY_CATEGORY_DISCOUNT:
        handleProductApplyCategoryDiscount(session, message);
        break;
    case Protocol::MessageType::ORDER_CREATE:
        handleOrderCreate(session, message);
        break;
    case Protocol::MessageType::ORDER_DIRECT_PURCHASE:
        handleDirectPurchase(session, message);
        break;
    case Protocol::MessageType::ORDER_GET_BY_USER:
        handleOrderGetByUser(session, message);
        break;
    case Protocol::MessageType::INVENTORY_LOCK:
        handleInventoryLock(session, message);
        break;
    case Protocol::MessageType::INVENTORY_UNLOCK:
        handleInventoryUnlock(session, message);
        break;
    default:
        sendErrorResponse(session, "不支持的消息类型");
        break;
    }
}

// 用户登录处理
void NetworkServer::handleUserLogin(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = message.getData("username");
    std::string password = message.getData("password");

    // 查找用户
    User *user = User::findUser(users, username);
    if (!user || user->getPassword() != password)
    {
        sendErrorResponse(session, "用户名或密码错误");
        return;
    } // 检查该用户是否已经在其他客户端登录
    std::shared_ptr<ClientSession> existingSession = findSessionByUsername(username);
    if (existingSession && existingSession->getSessionId() != session->getSessionId())
    {
        sendErrorResponse(session, "该用户已经登录");
        std::cout << "用户 " << username << " 尝试重复登录，已拒绝" << std::endl;
        return;
    }

    // 设置会话用户信息
    session->setUsername(username);
    std::string userTypeStr = user->getUserType();
    if (userTypeStr == "customer")
    {
        session->setUserType(Protocol::UserType::CUSTOMER);
    }
    else if (userTypeStr == "seller")
    {
        session->setUserType(Protocol::UserType::SELLER);
    }
    else if (userTypeStr == "admin")
    {
        session->setUserType(Protocol::UserType::ADMIN);
    }

    // 准备响应数据
    std::map<std::string, std::string> responseData;
    responseData["sessionId"] = session->getSessionId();
    responseData["userData"] = convertToUserData(user).serialize();

    sendSuccessResponse(session, responseData);
    std::cout << "用户登录成功: " << username << std::endl;
}

// 用户注册处理
void NetworkServer::handleUserRegister(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = message.getData("username");
    std::string password = message.getData("password");
    int userTypeInt = std::stoi(message.getData("userType"));

    // 检查用户名是否已存在
    if (User::isUsernameExists(users, username))
    {
        sendErrorResponse(session, "用户名已存在");
        return;
    }

    // 创建新用户
    User *newUser = nullptr;
    switch (static_cast<Protocol::UserType>(userTypeInt))
    {
    case Protocol::UserType::CUSTOMER:
        newUser = new Customer("./server_data/carts");
        break;
    case Protocol::UserType::SELLER:
        newUser = new Seller();
        break;
    case Protocol::UserType::ADMIN:
        newUser = new Admin();
        break;
    }

    if (newUser)
    {
        newUser->setUsername(username);
        newUser->setPassword(password);
        newUser->setBalance(0.0);
        users.push_back(newUser);

        saveUserData();
        sendSuccessResponse(session);
        std::cout << "新用户注册成功: " << username << std::endl;
    }
    else
    {
        sendErrorResponse(session, "创建用户失败");
    }
}

// 用户登出处理
void NetworkServer::handleUserLogout(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    session->setUsername("");
    session->setUserType(Protocol::UserType::CUSTOMER);
    sendSuccessResponse(session);
    std::cout << "用户登出成功" << std::endl;
}

// 获取用户信息处理
void NetworkServer::handleUserGetInfo(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 查找用户
    User *user = User::findUser(users, username);
    if (!user)
    {
        sendErrorResponse(session, "用户不存在");
        return;
    }

    // 准备响应数据
    std::map<std::string, std::string> responseData;
    responseData["userData"] = convertToUserData(user).serialize();

    sendDataResponse(session, responseData);
    std::cout << "用户 " << username << " 获取用户信息成功" << std::endl;
}

// 用户余额更新处理
void NetworkServer::handleUserUpdateBalance(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 获取充值金额
    std::string amountStr = message.getData("amount");
    double amount = 0.0;
    try
    {
        amount = std::stod(amountStr);
    }
    catch (const std::exception &e)
    {
        sendErrorResponse(session, "无效的充值金额");
        return;
    }

    if (amount <= 0)
    {
        sendErrorResponse(session, "充值金额必须大于0");
        return;
    }

    // 查找用户
    User *user = User::findUser(users, username);
    if (!user)
    {
        sendErrorResponse(session, "用户不存在");
        return;
    }

    // 执行充值
    if (user->deposit(amount))
    {
        // 保存用户数据
        saveUserData();

        // 返回成功响应，包含新的余额
        std::map<std::string, std::string> responseData;
        responseData["newBalance"] = std::to_string(user->checkBalance());
        sendSuccessResponse(session, responseData);

        std::cout << "用户 " << username << " 充值成功，金额: " << amount
                  << "，当前余额: " << user->checkBalance() << std::endl;
    }
    else
    {
        sendErrorResponse(session, "充值失败");
    }
}

// 用户密码修改处理
void NetworkServer::handleUserChangePassword(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 获取旧密码和新密码
    std::string oldPassword = message.getData("oldPassword");
    std::string newPassword = message.getData("newPassword");

    if (oldPassword.empty() || newPassword.empty())
    {
        sendErrorResponse(session, "密码不能为空");
        return;
    }

    // 查找用户
    User *user = User::findUser(users, username);
    if (!user)
    {
        sendErrorResponse(session, "用户不存在");
        return;
    }

    // 执行密码修改
    if (user->changePassword(oldPassword, newPassword))
    {
        // 保存用户数据
        saveUserData();

        sendSuccessResponse(session);

        std::cout << "用户 " << username << " 密码修改成功" << std::endl;
    }
    else
    {
        sendErrorResponse(session, "原密码错误");
    }
}

// 商品获取处理
void NetworkServer::handleProductGetAll(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::vector<Product *> allProducts = store->getProducts();
    std::vector<Protocol::ProductData> productDataList = convertToProductDataList(allProducts);

    std::map<std::string, std::string> responseData;
    responseData["count"] = std::to_string(productDataList.size());

    for (size_t i = 0; i < productDataList.size(); ++i)
    {
        responseData["product_" + std::to_string(i)] = productDataList[i].serialize();
    }
    sendDataResponse(session, responseData);
}

// 商品搜索处理
void NetworkServer::handleProductSearch(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string keyword = message.getData("keyword");
    if (keyword.empty())
    {
        sendErrorResponse(session, "搜索关键词不能为空");
        return;
    }

    // 使用Store的搜索功能
    std::vector<Product *> searchResults = store->searchProductsByName(keyword);
    std::vector<Protocol::ProductData> productDataList = convertToProductDataList(searchResults);

    std::map<std::string, std::string> responseData;
    responseData["count"] = std::to_string(productDataList.size());

    for (size_t i = 0; i < productDataList.size(); ++i)
    {
        responseData["product_" + std::to_string(i)] = productDataList[i].serialize();
    }

    sendDataResponse(session, responseData);
    std::cout << "商品搜索完成，关键词: \"" << keyword << "\"，找到 " << productDataList.size() << " 个结果" << std::endl;
}

// 购物车获取处理
void NetworkServer::handleCartGet(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 找到用户
    User *user = User::findUser(users, username);
    Customer *customer = dynamic_cast<Customer *>(user);
    if (!customer)
    {
        sendErrorResponse(session, "不是消费者用户");
        return;
    }

    // 获取购物车数据
    std::vector<Protocol::CartItemData> cartData;
    for (const auto &item : customer->shoppingCartItems)
    {
        Protocol::CartItemData cartItem;
        cartItem.productId = item.productId;
        cartItem.productName = item.productName;
        cartItem.quantity = item.quantity;
        cartItem.priceAtAddition = item.priceAtAddition;
        cartItem.sellerUsername = item.sellerUsername;
        cartData.push_back(cartItem);
    }

    std::map<std::string, std::string> responseData;
    responseData["count"] = std::to_string(cartData.size());

    for (size_t i = 0; i < cartData.size(); ++i)
    {
        responseData["item_" + std::to_string(i)] = cartData[i].serialize();
    }

    sendDataResponse(session, responseData);
}

// 购物车添加商品处理
void NetworkServer::handleCartAddItem(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 找到用户
    User *user = User::findUser(users, username);
    Customer *customer = dynamic_cast<Customer *>(user);
    if (!customer)
    {
        sendErrorResponse(session, "不是消费者用户");
        return;
    }

    std::string productId = message.getData("productId");
    int quantity = std::stoi(message.getData("quantity"));

    // 查找商品
    Product *product = store->findProductByName(productId);
    if (!product)
    {
        sendErrorResponse(session, "商品不存在");
        return;
    }

    // 检查库存
    if (product->getQuantity() < quantity)
    {
        sendErrorResponse(session, "库存不足");
        return;
    } // 添加到购物车 - 使用正确的方法名和参数
    if (customer->addToCart(*product, quantity))
    {
        // 添加成功，需要将商品信息保存到购物车文件
        customer->saveCartToFile();
        sendSuccessResponse(session);
        std::cout << "商品已添加到购物车: " << productId << ", 数量: " << quantity << std::endl;
    }
    else
    {
        sendErrorResponse(session, "添加商品到购物车失败");
    }
}

// 购物车移除商品处理
void NetworkServer::handleCartRemoveItem(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 找到用户
    User *user = User::findUser(users, username);
    Customer *customer = dynamic_cast<Customer *>(user);
    if (!customer)
    {
        sendErrorResponse(session, "不是消费者用户");
        return;
    }

    std::string productId = message.getData("productId"); // 从购物车移除商品 - 使用正确的方法名
    if (customer->removeCartItem(productId))
    {
        customer->saveCartToFile();
        sendSuccessResponse(session);
        std::cout << "商品已从购物车移除: " << productId << std::endl;
    }
    else
    {
        sendErrorResponse(session, "移除商品失败");
    }
}

// 购物车清空处理
void NetworkServer::handleCartClear(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 找到用户
    User *user = User::findUser(users, username);
    Customer *customer = dynamic_cast<Customer *>(user);
    if (!customer)
    {
        sendErrorResponse(session, "不是消费者用户");
        return;
    }

    // 清空购物车 - 使用正确的方法名
    customer->clearCartAndFile();

    sendSuccessResponse(session);
    std::cout << "购物车已清空，用户: " << username << std::endl;
}

// 购物车商品数量更新处理
void NetworkServer::handleCartUpdateItem(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 找到用户
    User *user = User::findUser(users, username);
    Customer *customer = dynamic_cast<Customer *>(user);
    if (!customer)
    {
        sendErrorResponse(session, "不是消费者用户");
        return;
    }

    std::string productId = message.getData("productId");
    int newQuantity = 0;
    try
    {
        newQuantity = std::stoi(message.getData("quantity"));
    }
    catch (const std::exception &e)
    {
        sendErrorResponse(session, "无效的数量");
        return;
    }

    // 检查数量是否有效
    if (newQuantity < 0)
    {
        sendErrorResponse(session, "数量不能为负数");
        return;
    }

    // 如果新数量为0，则移除商品
    if (newQuantity == 0)
    {
        if (customer->removeCartItem(productId))
        {
            customer->saveCartToFile();
            sendSuccessResponse(session);
            std::cout << "商品已从购物车移除: " << productId << std::endl;
        }
        else
        {
            sendErrorResponse(session, "移除商品失败");
        }
        return;
    }

    // 查找商品以检查库存
    Product *product = store->findProductByName(productId);
    if (!product)
    {
        sendErrorResponse(session, "商品不存在");
        return;
    }

    // 检查库存
    if (product->getQuantity() < newQuantity)
    {
        sendErrorResponse(session, "库存不足");
        return;
    }

    // 更新购物车商品数量
    if (customer->updateCartItemQuantity(productId, newQuantity, *store))
    {
        customer->saveCartToFile();
        sendSuccessResponse(session);
        std::cout << "购物车商品数量已更新: " << productId << ", 新数量: " << newQuantity << std::endl;
    }
    else
    {
        sendErrorResponse(session, "更新购物车商品数量失败");
    }
}

// 添加商品处理
void NetworkServer::handleProductAdd(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 找到用户
    User *user = User::findUser(users, username);
    Seller *seller = dynamic_cast<Seller *>(user);
    if (!seller)
    {
        sendErrorResponse(session, "不是商家用户");
        return;
    }
    // 解析商品数据
    std::string productDataStr = message.getData("productData");
    Protocol::ProductData productData = Protocol::ProductData::deserialize(productDataStr);    // 根据商品类型创建具体的商品对象，使用从客户端发送的attributes数据
    bool success = false;
    if (productData.type == "Book")
    {
        // 对于图书，从attributes中获取作者和ISBN
        std::string author = "Unknown Author";
        std::string isbn = "N/A";
        auto authorIt = productData.attributes.find("author");
        auto isbnIt = productData.attributes.find("isbn");
        if (authorIt != productData.attributes.end()) {
            author = authorIt->second;
        }
        if (isbnIt != productData.attributes.end()) {
            isbn = isbnIt->second;
        }
        
        success = store->createBook(seller, productData.name, productData.description,
                                    productData.price, productData.quantity,
                                    author, isbn);
    }
    else if (productData.type == "Clothing")
    {
        // 对于服装，从attributes中获取尺码和颜色
        std::string size = "M";
        std::string color = "Default";
        auto sizeIt = productData.attributes.find("size");
        auto colorIt = productData.attributes.find("color");
        if (sizeIt != productData.attributes.end()) {
            size = sizeIt->second;
        }
        if (colorIt != productData.attributes.end()) {
            color = colorIt->second;
        }
        
        success = store->createClothing(seller, productData.name, productData.description,
                                        productData.price, productData.quantity,
                                        size, color);
    }
    else if (productData.type == "Food")
    {
        // 对于食品，从attributes中获取过期日期
        std::string expirationDate = "2025-12-31";
        auto expIt = productData.attributes.find("expirationDate");
        if (expIt != productData.attributes.end()) {
            expirationDate = expIt->second;
        }
        
        success = store->createFood(seller, productData.name, productData.description,
                                    productData.price, productData.quantity,
                                    expirationDate);
    }
    else
    {
        // 对于通用商品，从attributes中获取类别标签
        std::string categoryTag = productData.type;
        auto categoryIt = productData.attributes.find("categoryTag");
        if (categoryIt != productData.attributes.end()) {
            categoryTag = categoryIt->second;
        }
        
        // 通用商品类型始终设为"Generic"
        success = store->createGenericProduct(seller, productData.name, productData.description,
                                              productData.price, productData.quantity,
                                              categoryTag);
    }

    // 检查创建结果
    if (success)
    {
        sendSuccessResponse(session);
        std::cout << "商品添加成功: " << productData.name << "，商家: " << seller->getUsername() << std::endl;
    }
    else
    {
        sendErrorResponse(session, "添加商品失败");
    }
}

// 订单创建处理
void NetworkServer::handleOrderCreate(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 找到用户
    User *user = User::findUser(users, username);
    Customer *customer = dynamic_cast<Customer *>(user);
    if (!customer)
    {
        sendErrorResponse(session, "不是消费者用户");
        return;
    }

    // 检查购物车是否为空
    if (customer->shoppingCartItems.empty())
    {
        sendErrorResponse(session, "购物车为空");
        return;
    }
    // 创建订单
    Order newOrder(customer->getUsername());
    for (const auto &cartItem : customer->shoppingCartItems)
    { // 将CartItem转换为OrderItem
        OrderItem orderItem(cartItem.productId, cartItem.productName,
                            cartItem.quantity, cartItem.priceAtAddition,
                            cartItem.sellerUsername);
        newOrder.addItem(orderItem);
    }

    // 提交订单给订单管理器
    auto submittedOrder = orderManager->submitOrderRequest(newOrder);
    if (submittedOrder)
    {
        // 立即处理订单（同步处理）
        orderManager->processNextOrder(*store, users);
        // 清空购物车 - 使用正确的方法名
        customer->clearCartAndFile();

        std::map<std::string, std::string> responseData;
        responseData["orderId"] = submittedOrder->getOrderId();
        sendSuccessResponse(session, responseData);

        std::cout << "订单创建成功: " << submittedOrder->getOrderId() << std::endl;
    }
    else
    {
        sendErrorResponse(session, "订单创建失败");
    }
}

// 直接购买处理
void NetworkServer::handleDirectPurchase(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 找到用户
    User *user = User::findUser(users, username);
    Customer *customer = dynamic_cast<Customer *>(user);
    if (!customer)
    {
        sendErrorResponse(session, "不是消费者用户");
        return;
    }

    // 获取请求参数
    std::string productId = message.getData("productId");
    int quantity = std::stoi(message.getData("quantity"));

    // 查找商品
    Product *product = store->findProductByName(productId);
    if (!product)
    {
        sendErrorResponse(session, "商品不存在");
        return;
    }

    // 检查库存
    if (product->getQuantity() < quantity)
    {
        sendErrorResponse(session, "库存不足");
        return;
    }

    // 创建直接购买订单（不涉及购物车）
    Order newOrder(customer->getUsername());
    OrderItem orderItem(product->getName(), product->getName(),
                        quantity, product->getPrice(),
                        product->getSellerUsername());
    newOrder.addItem(orderItem);

    // 提交订单给订单管理器
    auto submittedOrder = orderManager->submitOrderRequest(newOrder);
    if (submittedOrder)
    {
        // 立即处理订单（同步处理）
        orderManager->processNextOrder(*store, users);

        std::map<std::string, std::string> responseData;
        responseData["orderId"] = submittedOrder->getOrderId();
        sendSuccessResponse(session, responseData);

        std::cout << "直接购买订单创建成功: " << submittedOrder->getOrderId() << std::endl;
    }
    else
    {
        sendErrorResponse(session, "直接购买订单创建失败");
    }
}

// 获取用户订单处理
void NetworkServer::handleOrderGetByUser(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 获取用户的所有订单
    std::vector<std::shared_ptr<Order>> pendingOrders, completedOrders;
    orderManager->getUserOrders(username, pendingOrders, completedOrders);

    // 合并所有订单
    std::vector<std::shared_ptr<Order>> allOrders;
    allOrders.insert(allOrders.end(), pendingOrders.begin(), pendingOrders.end());
    allOrders.insert(allOrders.end(), completedOrders.begin(), completedOrders.end());

    // 转换为协议数据
    std::vector<Protocol::OrderData> orderDataList;
    for (const auto &order : allOrders)
    {
        Protocol::OrderData orderData;
        orderData.orderId = order->getOrderId();
        orderData.customerUsername = order->getCustomerUsername();
        orderData.totalAmount = order->getTotalAmount();
        orderData.timestamp = order->getTimestamp();

        // 转换订单状态
        std::string status = order->getStatus();
        if (status == "PENDING")
        {
            orderData.status = Protocol::OrderStatus::PENDING;
        }
        else if (status == "COMPLETED")
        {
            orderData.status = Protocol::OrderStatus::COMPLETED;
        }
        else if (status == "CANCELLED_STOCK")
        {
            orderData.status = Protocol::OrderStatus::CANCELLED_STOCK;
        }
        else if (status == "CANCELLED_FUNDS")
        {
            orderData.status = Protocol::OrderStatus::CANCELLED_FUNDS;
        }
        else if (status == "CANCELLED_USER")
        {
            orderData.status = Protocol::OrderStatus::CANCELLED_USER;
        }
        else
        {
            orderData.status = Protocol::OrderStatus::PENDING;
        }

        // 转换订单项
        for (const auto &item : order->getItems())
        {
            Protocol::CartItemData cartItem;
            cartItem.productId = item.productId;
            cartItem.productName = item.productName;
            cartItem.quantity = item.quantity;
            cartItem.priceAtAddition = item.priceAtPurchase;
            cartItem.sellerUsername = item.sellerUsername;
            orderData.items.push_back(cartItem);
        }

        orderDataList.push_back(orderData);
    }

    // 发送响应
    std::map<std::string, std::string> responseData;
    responseData["count"] = std::to_string(orderDataList.size());

    for (size_t i = 0; i < orderDataList.size(); ++i)
    {
        responseData["order_" + std::to_string(i)] = orderDataList[i].serialize();
    }
    sendDataResponse(session, responseData);
}

// 商家管理处理函数
void NetworkServer::handleProductManagePrice(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 找到用户
    User *user = User::findUser(users, username);
    Seller *seller = dynamic_cast<Seller *>(user);
    if (!seller)
    {
        sendErrorResponse(session, "不是商家用户");
        return;
    }

    // 获取参数
    std::string productName = message.getData("productName");
    double newPrice = 0.0;
    try
    {
        newPrice = std::stod(message.getData("newPrice"));
    }
    catch (const std::exception &e)
    {
        sendErrorResponse(session, "无效的价格");
        return;
    }

    // 调用Store方法修改价格
    if (store->manageProductPrice(seller, productName, newPrice))
    {
        sendSuccessResponse(session);
        std::cout << "商家 " << username << " 修改商品 " << productName << " 价格为 " << newPrice << std::endl;
    }
    else
    {
        sendErrorResponse(session, "修改价格失败");
    }
}

void NetworkServer::handleProductManageQuantity(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 找到用户
    User *user = User::findUser(users, username);
    Seller *seller = dynamic_cast<Seller *>(user);
    if (!seller)
    {
        sendErrorResponse(session, "不是商家用户");
        return;
    }

    // 获取参数
    std::string productName = message.getData("productName");
    int newQuantity = 0;
    try
    {
        newQuantity = std::stoi(message.getData("newQuantity"));
    }
    catch (const std::exception &e)
    {
        sendErrorResponse(session, "无效的数量");
        return;
    }

    // 调用Store方法修改库存
    if (store->manageProductQuantity(seller, productName, newQuantity))
    {
        sendSuccessResponse(session);
        std::cout << "商家 " << username << " 修改商品 " << productName << " 库存为 " << newQuantity << std::endl;
    }
    else
    {
        sendErrorResponse(session, "修改库存失败");
    }
}

void NetworkServer::handleProductManageDiscount(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 找到用户
    User *user = User::findUser(users, username);
    Seller *seller = dynamic_cast<Seller *>(user);
    if (!seller)
    {
        sendErrorResponse(session, "不是商家用户");
        return;
    }

    // 获取参数
    std::string productName = message.getData("productName");
    double newDiscount = 0.0;
    try
    {
        newDiscount = std::stod(message.getData("newDiscount"));
    }
    catch (const std::exception &e)
    {
        sendErrorResponse(session, "无效的折扣");
        return;
    }

    // 调用Store方法修改折扣
    if (store->manageProductDiscount(seller, productName, newDiscount))
    {
        sendSuccessResponse(session);
        std::cout << "商家 " << username << " 修改商品 " << productName << " 折扣为 " << (newDiscount * 100) << "%" << std::endl;
    }
    else
    {
        sendErrorResponse(session, "修改折扣失败");
    }
}

void NetworkServer::handleProductApplyCategoryDiscount(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 找到用户
    User *user = User::findUser(users, username);
    Seller *seller = dynamic_cast<Seller *>(user);
    if (!seller)
    {
        sendErrorResponse(session, "不是商家用户");
        return;
    }

    // 获取参数
    std::string category = message.getData("category");
    double discount = 0.0;
    try
    {
        discount = std::stod(message.getData("discount"));
    }
    catch (const std::exception &e)
    {
        sendErrorResponse(session, "无效的折扣");
        return;
    }

    // 调用Store方法应用类别折扣
    if (store->applyCategoryDiscount(seller, category, discount))
    {
        sendSuccessResponse(session);
        std::cout << "商家 " << username << " 为类别 " << category << " 应用了 " << (discount * 100) << "% 折扣" << std::endl;
    }
    else
    {
        sendErrorResponse(session, "应用类别折扣失败");
    }
}

// 库存锁定处理
void NetworkServer::handleInventoryLock(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 检查是否有单个商品的锁定请求
    std::string productId = message.getData("productId");
    if (!productId.empty())
    {
        // 单个商品锁定
        int quantity = 0;
        try
        {
            quantity = std::stoi(message.getData("quantity"));
        }
        catch (const std::exception &e)
        {
            sendErrorResponse(session, "无效的数量");
            return;
        }

        // 锁定库存
        if (store->lockInventory(productId, quantity))
        {
            sendSuccessResponse(session);
            std::cout << "用户 " << username << " 锁定库存成功: " << productId << ", 数量: " << quantity << std::endl;
        }
        else
        {
            sendErrorResponse(session, "库存锁定失败");
        }
    }
    else
    {
        // 多个商品锁定（购物车）
        std::string itemCountStr = message.getData("itemCount");
        if (itemCountStr.empty())
        {
            sendErrorResponse(session, "缺少商品数量信息");
            return;
        }

        int itemCount = 0;
        try
        {
            itemCount = std::stoi(itemCountStr);
        }
        catch (const std::exception &e)
        {
            sendErrorResponse(session, "无效的商品数量");
            return;
        }

        // 收集所有商品和数量
        std::vector<std::pair<std::string, int>> items;
        for (int i = 0; i < itemCount; ++i)
        {
            std::string prodId = message.getData("productId_" + std::to_string(i));
            std::string quantityStr = message.getData("quantity_" + std::to_string(i));

            if (prodId.empty() || quantityStr.empty())
            {
                sendErrorResponse(session, "商品信息不完整");
                return;
            }

            int qty = 0;
            try
            {
                qty = std::stoi(quantityStr);
            }
            catch (const std::exception &e)
            {
                sendErrorResponse(session, "无效的商品数量");
                return;
            }

            items.push_back({prodId, qty});
        }

        // 锁定所有商品的库存
        bool allLocked = true;
        for (const auto &item : items)
        {
            if (!store->lockInventory(item.first, item.second))
            {
                allLocked = false;
                // 回滚已锁定的库存
                for (int j = 0; j < &item - &items[0]; ++j)
                {
                    store->unlockInventory(items[j].first, items[j].second);
                }
                break;
            }
        }

        if (allLocked)
        {
            sendSuccessResponse(session);
            std::cout << "用户 " << username << " 购物车库存锁定成功，商品数量: " << itemCount << std::endl;
        }
        else
        {
            sendErrorResponse(session, "购物车库存锁定失败");
        }
    }
}

void NetworkServer::handleInventoryUnlock(std::shared_ptr<ClientSession> session, const Protocol::Message &message)
{
    std::string username = session->getUsername();
    if (username.empty())
    {
        sendErrorResponse(session, "用户未登录");
        return;
    }

    // 检查是否有单个商品的解锁请求
    std::string productId = message.getData("productId");
    if (!productId.empty())
    {
        // 单个商品解锁
        int quantity = 0;
        try
        {
            quantity = std::stoi(message.getData("quantity"));
        }
        catch (const std::exception &e)
        {
            sendErrorResponse(session, "无效的数量");
            return;
        }

        // 解锁库存
        if (store->unlockInventory(productId, quantity))
        {
            sendSuccessResponse(session);
            std::cout << "用户 " << username << " 解锁库存成功: " << productId << ", 数量: " << quantity << std::endl;
        }
        else
        {
            sendErrorResponse(session, "库存解锁失败");
        }
    }
    else
    {
        // 多个商品解锁（购物车）
        std::string itemCountStr = message.getData("itemCount");
        if (itemCountStr.empty())
        {
            sendErrorResponse(session, "缺少商品数量信息");
            return;
        }

        int itemCount = 0;
        try
        {
            itemCount = std::stoi(itemCountStr);
        }
        catch (const std::exception &e)
        {
            sendErrorResponse(session, "无效的商品数量");
            return;
        }

        // 收集所有商品和数量并解锁
        for (int i = 0; i < itemCount; ++i)
        {
            std::string prodId = message.getData("productId_" + std::to_string(i));
            std::string quantityStr = message.getData("quantity_" + std::to_string(i));

            if (!prodId.empty() && !quantityStr.empty())
            {
                try
                {
                    int qty = std::stoi(quantityStr);
                    store->unlockInventory(prodId, qty);
                }
                catch (const std::exception &e)
                {
                    // 继续处理其他商品，即使某个商品解锁失败
                    std::cerr << "解锁商品 " << prodId << " 失败: " << e.what() << std::endl;
                }
            }
        }

        sendSuccessResponse(session);
        std::cout << "用户 " << username << " 购物车库存解锁完成，商品数量: " << itemCount << std::endl;
    }
}

// 响应发送辅助方法
void NetworkServer::sendSuccessResponse(std::shared_ptr<ClientSession> session, const std::map<std::string, std::string> &data)
{
    Protocol::Message response(Protocol::MessageType::RESPONSE_SUCCESS, session->getSessionId());
    for (const auto &pair : data)
    {
        response.setData(pair.first, pair.second);
    }
    session->sendMessage(response);
}

void NetworkServer::sendErrorResponse(std::shared_ptr<ClientSession> session, const std::string &error)
{
    Protocol::Message response(Protocol::MessageType::RESPONSE_ERROR, session->getSessionId());
    response.setData("error", error);
    session->sendMessage(response);
}

void NetworkServer::sendDataResponse(std::shared_ptr<ClientSession> session, const std::map<std::string, std::string> &data)
{
    Protocol::Message response(Protocol::MessageType::RESPONSE_DATA, session->getSessionId());
    for (const auto &pair : data)
    {
        response.setData(pair.first, pair.second);
    }
    session->sendMessage(response);
}

// 数据转换方法
Protocol::UserData NetworkServer::convertToUserData(const User *user)
{
    Protocol::UserData userData;
    userData.username = user->getUsername();
    userData.password = user->getPassword();
    userData.balance = user->checkBalance();

    std::string userType = user->getUserType();
    if (userType == "customer")
    {
        userData.userType = Protocol::UserType::CUSTOMER;
    }
    else if (userType == "seller")
    {
        userData.userType = Protocol::UserType::SELLER;
    }
    else if (userType == "admin")
    {
        userData.userType = Protocol::UserType::ADMIN;
    }

    return userData;
}

Protocol::ProductData NetworkServer::convertToProductData(const Product *product)
{
    Protocol::ProductData productData;
    productData.id = product->getName(); // 使用名称作为ID
    productData.name = product->getName();
    productData.description = product->getDescription();

    // 修复Generic产品显示问题：使用getUserCategory()显示实际类别名称
    std::string userCategory = product->getUserCategory();
    if (!userCategory.empty())
    {
        productData.type = userCategory; // 显示实际类别名称（如自定义标签）
    }
    else
    {
        productData.type = product->getType(); // 回退到基本类型
    }
    // 修复价格显示：发送折扣后的实际价格
    productData.price = product->getPrice();                 // 使用 getPrice() 获取折扣后价格
    productData.originalPrice = product->getOriginalPrice(); // 添加原价字段
    productData.quantity = product->getQuantity();
    productData.discountRate = product->getDiscountRate();
    productData.sellerUsername = product->getSellerUsername();

    return productData;
}

std::vector<Protocol::ProductData> NetworkServer::convertToProductDataList(const std::vector<Product *> &products)
{
    std::vector<Protocol::ProductData> result;
    for (const Product *product : products)
    {
        result.push_back(convertToProductData(product));
    }
    return result;
}

// 数据持久化方法
void NetworkServer::loadUserData()
{
    users = User::loadUsersFromFile(userFile);
    std::cout << "已加载 " << users.size() << " 个用户数据" << std::endl;
}

void NetworkServer::saveUserData()
{
    User::saveUsersToFile(users, userFile);
    std::cout << "用户数据已保存" << std::endl;
}

void NetworkServer::initializeStore()
{
    store = std::make_unique<Store>(storeDir);
    store->loadAllProducts();
    std::cout << "商店数据已初始化" << std::endl;
}

void NetworkServer::initializeOrderManager()
{
    orderManager = std::make_unique<OrderManager>(orderDir);
    // 注意：这里不启动处理线程，因为服务端会同步处理订单
    std::cout << "订单管理器已初始化" << std::endl;
}
