#include "client.h"
#include <iostream>
#include <chrono>

NetworkClient::NetworkClient(const std::string &address, int port)
    : serverAddress(address), serverPort(port), clientSocket(INVALID_SOCKET),
      isConnected(false), shouldStop(false)
{

    // 初始化Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup failed: " << result << std::endl;
    }
}

NetworkClient::~NetworkClient()
{
    disconnect();
    WSACleanup();
}

bool NetworkClient::connect()
{
    if (isConnected.load())
    {
        return true;
    }

    // 创建socket
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        return false;
    }

    // 设置服务器地址
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr);

    // 连接服务器
    if (::connect(clientSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        return false;
    }

    isConnected.store(true);
    shouldStop.store(false);

    // 启动接收线程
    receiveThread = std::thread(&NetworkClient::receiveLoop, this);

    std::cout << "已连接到服务器 " << serverAddress << ":" << serverPort << std::endl;
    return true;
}

void NetworkClient::disconnect()
{
    if (!isConnected.load())
    {
        return;
    }

    shouldStop.store(true);
    isConnected.store(false);

    if (clientSocket != INVALID_SOCKET)
    {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }

    if (receiveThread.joinable())
    {
        receiveThread.join();
    }

    // 清理消息队列
    std::lock_guard<std::mutex> lock(queueMutex);
    while (!responseQueue.empty())
    {
        responseQueue.pop();
    }

    std::cout << "已断开连接" << std::endl;
}

void NetworkClient::receiveLoop()
{
    while (!shouldStop.load() && isConnected.load())
    {
        std::string rawData = receiveRawData();
        if (rawData.empty())
        {
            break;
        }

        Protocol::Message message = Protocol::Message::deserialize(rawData);

        // 如果设置了回调函数，调用回调
        if (messageCallback)
        {
            messageCallback(message);
        }

        // 将消息放入队列
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            responseQueue.push(message);
        }
        queueCondition.notify_one();
    }
}

bool NetworkClient::sendRawData(const std::string &data)
{
    if (!isConnected.load())
    {
        return false;
    }

    // 发送数据长度
    uint32_t dataLength = htonl(static_cast<uint32_t>(data.length()));
    if (send(clientSocket, reinterpret_cast<const char *>(&dataLength), sizeof(dataLength), 0) == SOCKET_ERROR)
    {
        std::cerr << "Send length failed: " << WSAGetLastError() << std::endl;
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
            std::cerr << "Send data failed: " << WSAGetLastError() << std::endl;
            return false;
        }
        totalSent += sent;
    }

    return true;
}

std::string NetworkClient::receiveRawData()
{
    if (!isConnected.load())
    {
        return "";
    }

    // 接收数据长度
    uint32_t dataLength;
    int received = recv(clientSocket, reinterpret_cast<char *>(&dataLength), sizeof(dataLength), 0);
    if (received <= 0)
    {
        if (received == 0)
        {
            std::cout << "服务器关闭了连接" << std::endl;
        }
        else
        {
            std::cerr << "Receive length failed: " << WSAGetLastError() << std::endl;
        }
        isConnected.store(false);
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
            std::cerr << "Receive data failed: " << WSAGetLastError() << std::endl;
            isConnected.store(false);
            return "";
        }
        totalReceived += received;
    }

    return data;
}

bool NetworkClient::sendMessage(const Protocol::Message &message)
{
    std::string serialized = message.serialize();
    return sendRawData(serialized);
}

Protocol::Message NetworkClient::waitForResponse(Protocol::MessageType expectedType, int timeoutMs)
{
    auto start = std::chrono::steady_clock::now();

    while (true)
    {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            // 减少等待时间，避免阻塞UI
            if (queueCondition.wait_for(lock, std::chrono::milliseconds(50), [this]
                                        { return !responseQueue.empty(); }))
            {
                Protocol::Message msg = responseQueue.front();
                if (msg.type == expectedType || expectedType == Protocol::MessageType::RESPONSE_DATA)
                {
                    responseQueue.pop();
                    return msg;
                }
                // 如果不是期望的消息类型，但是是错误消息，也应该返回
                if (msg.type == Protocol::MessageType::RESPONSE_ERROR)
                {
                    responseQueue.pop();
                    return msg;
                }
            }
        }

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > timeoutMs)
        {
            break;
        }

        // 如果连接断开，立即返回
        if (!isConnected.load())
        {
            break;
        }
    }

    return Protocol::Message(); // 超时返回空消息
}

bool NetworkClient::hasResponse() const
{
    std::lock_guard<std::mutex> lock(queueMutex);
    return !responseQueue.empty();
}

Protocol::Message NetworkClient::getNextResponse()
{
    std::lock_guard<std::mutex> lock(queueMutex);
    if (!responseQueue.empty())
    {
        Protocol::Message msg = responseQueue.front();
        responseQueue.pop();
        return msg;
    }
    return Protocol::Message();
}

void NetworkClient::setMessageCallback(std::function<void(const Protocol::Message &)> callback)
{
    messageCallback = callback;
}

// 用户操作实现
bool NetworkClient::login(const std::string &username, const std::string &password, Protocol::UserData &userData)
{
    std::string errorMessage;
    return loginWithError(username, password, userData, errorMessage);
}

bool NetworkClient::loginWithError(const std::string &username, const std::string &password, Protocol::UserData &userData, std::string &errorMessage)
{
    Protocol::Message request(Protocol::MessageType::USER_LOGIN);
    request.setData("username", username);
    request.setData("password", password);

    if (!sendMessage(request))
    {
        errorMessage = "无法发送登录请求";
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_SUCCESS);
    if (response.type == Protocol::MessageType::RESPONSE_SUCCESS)
    {
        sessionId = response.getData("sessionId");
        userData = Protocol::UserData::deserialize(response.getData("userData"));
        return true;
    }
    else if (response.type == Protocol::MessageType::RESPONSE_ERROR)
    {
        errorMessage = response.getData("error");
        return false;
    }

    errorMessage = "登录超时或网络错误";
    return false;
}

bool NetworkClient::registerUser(const std::string &username, const std::string &password, Protocol::UserType userType)
{
    Protocol::Message request(Protocol::MessageType::USER_REGISTER);
    request.setData("username", username);
    request.setData("password", password);
    request.setData("userType", std::to_string(static_cast<int>(userType)));

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_SUCCESS);
    return response.type == Protocol::MessageType::RESPONSE_SUCCESS;
}

bool NetworkClient::logout()
{
    Protocol::Message request(Protocol::MessageType::USER_LOGOUT, sessionId);

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_SUCCESS);
    if (response.type == Protocol::MessageType::RESPONSE_SUCCESS)
    {
        sessionId.clear();
        return true;
    }

    return false;
}

// 商品操作实现
bool NetworkClient::getAllProducts(std::vector<Protocol::ProductData> &products)
{
    Protocol::Message request(Protocol::MessageType::PRODUCT_GET_ALL, sessionId);

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_DATA);
    if (response.type == Protocol::MessageType::RESPONSE_DATA)
    {
        products.clear();
        int count = std::stoi(response.getData("count"));
        for (int i = 0; i < count; ++i)
        {
            std::string productData = response.getData("product_" + std::to_string(i));
            products.push_back(Protocol::ProductData::deserialize(productData));
        }
        return true;
    }

    return false;
}

// 购物车操作实现
bool NetworkClient::getCart(std::vector<Protocol::CartItemData> &cartItems)
{
    Protocol::Message request(Protocol::MessageType::CART_GET, sessionId);

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_DATA);
    if (response.type == Protocol::MessageType::RESPONSE_DATA)
    {
        cartItems.clear();
        int count = std::stoi(response.getData("count"));
        for (int i = 0; i < count; ++i)
        {
            std::string itemData = response.getData("item_" + std::to_string(i));
            cartItems.push_back(Protocol::CartItemData::deserialize(itemData));
        }
        return true;
    }

    return false;
}

bool NetworkClient::addToCart(const std::string &productId, int quantity)
{
    Protocol::Message request(Protocol::MessageType::CART_ADD_ITEM, sessionId);
    request.setData("productId", productId);
    request.setData("quantity", std::to_string(quantity));

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_SUCCESS);
    return response.type == Protocol::MessageType::RESPONSE_SUCCESS;
}

bool NetworkClient::removeFromCart(const std::string &productId)
{
    Protocol::Message request(Protocol::MessageType::CART_REMOVE_ITEM, sessionId);
    request.setData("productId", productId);

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_SUCCESS);
    return response.type == Protocol::MessageType::RESPONSE_SUCCESS;
}

bool NetworkClient::clearCart()
{
    Protocol::Message request(Protocol::MessageType::CART_CLEAR, sessionId);

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_SUCCESS);
    return response.type == Protocol::MessageType::RESPONSE_SUCCESS;
}

bool NetworkClient::addProduct(const Protocol::ProductData &product)
{
    Protocol::Message request(Protocol::MessageType::PRODUCT_ADD, sessionId);
    request.setData("productData", product.serialize());

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_SUCCESS);
    return response.type == Protocol::MessageType::RESPONSE_SUCCESS;
}

// 订单操作实现
bool NetworkClient::createOrder(const std::vector<Protocol::CartItemData> &items, std::string &orderId)
{
    Protocol::Message request(Protocol::MessageType::ORDER_CREATE, sessionId);
    request.setData("itemCount", std::to_string(items.size()));

    for (size_t i = 0; i < items.size(); ++i)
    {
        request.setData("item_" + std::to_string(i), items[i].serialize());
    }

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_SUCCESS);
    if (response.type == Protocol::MessageType::RESPONSE_SUCCESS)
    {
        orderId = response.getData("orderId");
        return true;
    }
    return false;
}

bool NetworkClient::createDirectOrder(const std::string &productId, int quantity, std::string &orderId)
{
    Protocol::Message request(Protocol::MessageType::ORDER_DIRECT_PURCHASE, sessionId);
    request.setData("productId", productId);
    request.setData("quantity", std::to_string(quantity));

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_SUCCESS);
    if (response.type == Protocol::MessageType::RESPONSE_SUCCESS)
    {
        orderId = response.getData("orderId");
        return true;
    }

    return false;
}

bool NetworkClient::getUserOrders(std::vector<Protocol::OrderData> &orders)
{
    Protocol::Message request(Protocol::MessageType::ORDER_GET_BY_USER, sessionId);

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_DATA);
    if (response.type == Protocol::MessageType::RESPONSE_DATA)
    {
        orders.clear();
        int count = std::stoi(response.getData("count"));
        for (int i = 0; i < count; ++i)
        {
            std::string orderData = response.getData("order_" + std::to_string(i));
            // std::cerr << "Received order data: " << orderData << std::endl;
            orders.push_back(Protocol::OrderData::deserialize(orderData));
        }
        return true;
    }

    return false;
}

// 用户信息更新实现
bool NetworkClient::changePassword(const std::string &oldPassword, const std::string &newPassword)
{
    Protocol::Message request(Protocol::MessageType::USER_CHANGE_PASSWORD, sessionId);
    request.setData("oldPassword", oldPassword);
    request.setData("newPassword", newPassword);

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_SUCCESS);
    return response.type == Protocol::MessageType::RESPONSE_SUCCESS;
}

bool NetworkClient::updateBalance(double amount)
{
    Protocol::Message request(Protocol::MessageType::USER_UPDATE_BALANCE, sessionId);
    request.setData("amount", std::to_string(amount));

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_SUCCESS);
    return response.type == Protocol::MessageType::RESPONSE_SUCCESS;
}

// 商品搜索实现
bool NetworkClient::searchProducts(const std::string &keyword, std::vector<Protocol::ProductData> &products)
{
    Protocol::Message request(Protocol::MessageType::PRODUCT_SEARCH, sessionId);
    request.setData("keyword", keyword);

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_DATA);
    if (response.type == Protocol::MessageType::RESPONSE_DATA)
    {
        products.clear();
        int count = std::stoi(response.getData("count"));
        for (int i = 0; i < count; ++i)
        {
            std::string productData = response.getData("product_" + std::to_string(i));

            // std::cerr << "Received product data: " << productData << std::endl;
            products.push_back(Protocol::ProductData::deserialize(productData));
        }
        return true;
    }

    return false;
}

// 购物车商品更新实现
bool NetworkClient::updateCartItem(const std::string &productId, int newQuantity)
{
    Protocol::Message request(Protocol::MessageType::CART_UPDATE_ITEM, sessionId);
    request.setData("productId", productId);
    request.setData("quantity", std::to_string(newQuantity));

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_SUCCESS);
    return response.type == Protocol::MessageType::RESPONSE_SUCCESS;
}

// 商家管理操作实现
bool NetworkClient::manageProductPrice(const std::string &productName, double newPrice)
{
    Protocol::Message request(Protocol::MessageType::PRODUCT_MANAGE_PRICE, sessionId);
    request.setData("productName", productName);
    request.setData("newPrice", std::to_string(newPrice));

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_SUCCESS);
    return response.type == Protocol::MessageType::RESPONSE_SUCCESS;
}

bool NetworkClient::manageProductQuantity(const std::string &productName, int newQuantity)
{
    Protocol::Message request(Protocol::MessageType::PRODUCT_MANAGE_QUANTITY, sessionId);
    request.setData("productName", productName);
    request.setData("newQuantity", std::to_string(newQuantity));

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_SUCCESS);
    return response.type == Protocol::MessageType::RESPONSE_SUCCESS;
}

bool NetworkClient::manageProductDiscount(const std::string &productName, double newDiscount)
{
    Protocol::Message request(Protocol::MessageType::PRODUCT_MANAGE_DISCOUNT, sessionId);
    request.setData("productName", productName);
    request.setData("newDiscount", std::to_string(newDiscount));

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_SUCCESS);
    return response.type == Protocol::MessageType::RESPONSE_SUCCESS;
}

bool NetworkClient::applyCategoryDiscount(const std::string &category, double discount)
{
    Protocol::Message request(Protocol::MessageType::PRODUCT_APPLY_CATEGORY_DISCOUNT, sessionId);
    request.setData("category", category);
    request.setData("discount", std::to_string(discount));

    if (!sendMessage(request))
    {
        return false;
    }

    Protocol::Message response = waitForResponse(Protocol::MessageType::RESPONSE_SUCCESS);
    return response.type == Protocol::MessageType::RESPONSE_SUCCESS;
}
