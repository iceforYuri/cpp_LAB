#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

#include "protocol.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

#pragma comment(lib, "ws2_32.lib")

// 前向声明
class User;
class Store;
class OrderManager;
class Product;

// 客户端会话类
class ClientSession : public std::enable_shared_from_this<ClientSession>
{
private:
    SOCKET clientSocket;
    std::string sessionId;
    std::string username;
    Protocol::UserType userType;
    std::thread clientThread;
    std::atomic<bool> isActive;

public:
    ClientSession(SOCKET socket, const std::string &sid);
    ~ClientSession();

    SOCKET getSocket() const { return clientSocket; }
    std::string getSessionId() const { return sessionId; }
    std::string getUsername() const { return username; }
    Protocol::UserType getUserType() const { return userType; }
    bool isSessionActive() const { return isActive.load(); }

    void setUsername(const std::string &user) { username = user; }
    void setUserType(Protocol::UserType type) { userType = type; }

    bool sendMessage(const Protocol::Message &message);
    std::string receiveRawData();
    void startSession(class NetworkServer *server);
    void stopSession();

private:
    void sessionLoop(class NetworkServer *server);
    bool sendRawData(const std::string &data);
};

class NetworkServer
{
private:
    SOCKET serverSocket;
    int port;
    std::atomic<bool> isRunning;
    std::thread acceptThread;

    // 客户端会话管理
    std::vector<std::shared_ptr<ClientSession>> sessions;
    std::mutex sessionsMutex;

    // 业务逻辑组件
    std::vector<User *> users;
    std::unique_ptr<Store> store;
    std::unique_ptr<OrderManager> orderManager;

    // 数据文件路径
    std::string userFile;
    std::string storeDir;
    std::string orderDir;

    // 内部方法
    void acceptLoop();
    std::string generateSessionId();
    std::shared_ptr<ClientSession> findSession(const std::string &sessionId);
    void removeSession(const std::string &sessionId);
    std::shared_ptr<ClientSession> findSessionByUsername(const std::string &username);

    // 数据转换方法
    Protocol::UserData convertToUserData(const User *user);
    Protocol::ProductData convertToProductData(const Product *product);
    std::vector<Protocol::ProductData> convertToProductDataList(const std::vector<Product *> &products);

public:
    NetworkServer(int port = 8888);
    ~NetworkServer();

    // 服务器控制
    bool start();
    void stop();
    bool isServerRunning() const { return isRunning.load(); }

    // 客户端会话处理
    void handleClientMessage(std::shared_ptr<ClientSession> session, const Protocol::Message &message);

    // 用户管理处理
    void handleUserLogin(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleUserRegister(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleUserLogout(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleUserGetInfo(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleUserUpdateBalance(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleUserChangePassword(std::shared_ptr<ClientSession> session, const Protocol::Message &message);

    // 商品管理处理
    void handleProductGetAll(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleProductSearch(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleProductGetById(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleProductAdd(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleProductUpdate(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleProductDelete(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleProductUpdateStock(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleProductManagePrice(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleProductManageQuantity(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleProductManageDiscount(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleProductApplyCategoryDiscount(std::shared_ptr<ClientSession> session, const Protocol::Message &message);

    // 购物车管理处理
    void handleCartGet(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleCartAddItem(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleCartUpdateItem(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleCartRemoveItem(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleCartClear(std::shared_ptr<ClientSession> session, const Protocol::Message &message); // 订单管理处理
    void handleOrderCreate(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleDirectPurchase(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleOrderGetByUser(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleOrderGetById(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleOrderUpdateStatus(std::shared_ptr<ClientSession> session, const Protocol::Message &message);
    void handleOrderGetAll(std::shared_ptr<ClientSession> session, const Protocol::Message &message);

    // 响应发送辅助方法
    void sendSuccessResponse(std::shared_ptr<ClientSession> session, const std::map<std::string, std::string> &data = {});
    void sendErrorResponse(std::shared_ptr<ClientSession> session, const std::string &error);
    void sendDataResponse(std::shared_ptr<ClientSession> session, const std::map<std::string, std::string> &data);

    // 数据持久化
    void saveUserData();
    void loadUserData();
    void initializeStore();
    void initializeOrderManager();
};

#endif // NETWORK_SERVER_H
