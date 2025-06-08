#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include "protocol.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

class NetworkClient
{
private:
    SOCKET clientSocket;
    std::string serverAddress;
    int serverPort;
    std::string sessionId;

    // 消息处理
    std::thread receiveThread;
    std::atomic<bool> isConnected;
    std::atomic<bool> shouldStop;

    // 消息队列
    std::queue<Protocol::Message> responseQueue;
    mutable std::mutex queueMutex;
    std::condition_variable queueCondition;

    // 回调函数
    std::function<void(const Protocol::Message &)> messageCallback;

    // 内部方法
    void receiveLoop();
    bool sendRawData(const std::string &data);
    std::string receiveRawData();

public:
    NetworkClient(const std::string &address = "127.0.0.1", int port = 8888);
    ~NetworkClient();

    // 连接管理
    bool connect();
    void disconnect();
    bool isConnectionActive() const { return isConnected.load(); }

    // 消息发送
    bool sendMessage(const Protocol::Message &message); // 消息接收
    Protocol::Message waitForResponse(Protocol::MessageType expectedType, int timeoutMs = 2000);
    Protocol::Message waitForResponseBlocking(Protocol::MessageType expectedType); // 无超时限制版本
    bool hasResponse() const;
    Protocol::Message getNextResponse();

    // 设置消息回调
    void setMessageCallback(std::function<void(const Protocol::Message &)> callback);

    // 会话管理
    void setSessionId(const std::string &sid) { sessionId = sid; }
    std::string getSessionId() const { return sessionId; } // 用户操作
    bool login(const std::string &username, const std::string &password, Protocol::UserData &userData);
    bool loginWithError(const std::string &username, const std::string &password, Protocol::UserData &userData, std::string &errorMessage);
    bool registerUser(const std::string &username, const std::string &password, Protocol::UserType userType);
    bool logout();
    bool getUserInfo(Protocol::UserData &userData);
    bool updateBalance(double amount);
    bool changePassword(const std::string &oldPassword, const std::string &newPassword);

    // 商品操作
    bool getAllProducts(std::vector<Protocol::ProductData> &products);
    bool searchProducts(const std::string &keyword, std::vector<Protocol::ProductData> &products);
    bool getProductById(const std::string &productId, Protocol::ProductData &product);
    bool addProduct(const Protocol::ProductData &product);
    bool updateProduct(const Protocol::ProductData &product);
    bool deleteProduct(const std::string &productId);
    bool updateProductStock(const std::string &productId, int newStock);

    // 购物车操作
    bool getCart(std::vector<Protocol::CartItemData> &cartItems);
    bool addToCart(const std::string &productId, int quantity);
    bool updateCartItem(const std::string &productId, int newQuantity);
    bool removeFromCart(const std::string &productId);
    bool clearCart(); // 订单操作
    bool createOrder(const std::vector<Protocol::CartItemData> &items, std::string &orderId);
    bool createDirectOrder(const std::string &productId, int quantity, std::string &orderId);
    bool getUserOrders(std::vector<Protocol::OrderData> &orders);
    bool getOrderById(const std::string &orderId, Protocol::OrderData &order);
    bool updateOrderStatus(const std::string &orderId, Protocol::OrderStatus status);
    bool getAllOrders(std::vector<Protocol::OrderData> &orders); // 管理员功能

    // 库存锁定操作
    bool lockInventory(const std::string &productId, int quantity);
    bool unlockInventory(const std::string &productId, int quantity);
    bool lockCartInventory(const std::vector<Protocol::CartItemData> &items);
    bool unlockCartInventory(const std::vector<Protocol::CartItemData> &items);

    // 商家管理操作
    bool manageProductPrice(const std::string &productName, double newPrice);
    bool manageProductQuantity(const std::string &productName, int newQuantity);
    bool manageProductDiscount(const std::string &productName, double newDiscount);
    bool applyCategoryDiscount(const std::string &category, double discount);
};

#endif // NETWORK_CLIENT_H