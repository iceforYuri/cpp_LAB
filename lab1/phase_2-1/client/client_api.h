#ifndef CLIENT_API_H
#define CLIENT_API_H

#include <string>
#include <vector>
#include <memory>
#include "../network/json.hpp"
#include "../network/socket.h"
#include "../network/protocol.h"
#include "../user/user.h"
#include "../store/store.h"
#include "../order/order.h"

using json = nlohmann::json;

// 表示从服务器获取的产品信息
struct ProductInfo
{
    int id;
    std::string name;
    double price;
    std::string description;
    std::string seller;
};

// 表示从服务器获取的购物车项信息
struct CartItemInfo
{
    int productId;
    std::string name;
    double price;
    int quantity;
    double totalPrice;
};

// 表示从服务器获取的订单信息
struct OrderInfo
{
    int id;
    std::string date;
    std::string status;
    double totalAmount;
    std::string customer;            // 卖家视角可见
    std::vector<CartItemInfo> items; // 详情视图才有
};

class ClientAPI
{
private:
    ClientSocket socket;
    std::string token;
    std::string username;
    std::string userType;
    bool connected;
    bool authenticated;

    // 发送请求并接收响应
    json sendRequest(RequestType type, const json &data);

public:
    ClientAPI(const std::string &serverHost = "127.0.0.1", int serverPort = 8888);
    ~ClientAPI();

    // 连接到服务器
    bool connect();

    // 断开连接
    void disconnect();

    // 是否已连接
    bool isConnected() const { return connected; }

    // 是否已认证
    bool isAuthenticated() const { return authenticated; }

    // 获取用户名
    std::string getUsername() const { return username; }

    // 获取用户类型
    std::string getUserType() const { return userType; }

    // 用户相关的API方法
    bool login(const std::string &username, const std::string &password);
    bool registerUser(const std::string &username, const std::string &password, const std::string &userType);
    bool logout();

    // 产品相关的API方法
    std::vector<ProductInfo> getProducts();
    std::vector<ProductInfo> searchProducts(const std::string &keyword);
    ProductInfo getProductDetail(int productId);

    // 购物车相关的API方法
    bool addToCart(int productId, int quantity);
    bool removeFromCart(int productId);
    bool updateCartItemQuantity(int productId, int quantity);
    std::vector<CartItemInfo> getCart();

    // 订单相关的API方法
    bool createOrder(int &orderId);
    std::vector<OrderInfo> getOrders();
    OrderInfo getOrderDetail(int orderId);
    bool payOrder(int orderId);
    bool cancelOrder(int orderId);
    std::string getOrderStatus(int orderId);
};

#endif // CLIENT_API_H
