#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <vector>
#include <map>

// 网络协议定义
namespace Protocol
{

    // 消息类型枚举
    enum class MessageType : int
    {
        // 用户相关
        USER_LOGIN = 1000,
        USER_REGISTER = 1001,
        USER_LOGOUT = 1002,
        USER_GET_INFO = 1003,
        USER_UPDATE_BALANCE = 1004,
        USER_CHANGE_PASSWORD = 1005, // 商品相关
        PRODUCT_GET_ALL = 2000,
        PRODUCT_SEARCH = 2001,
        PRODUCT_GET_BY_ID = 2002,
        PRODUCT_ADD = 2003,
        PRODUCT_UPDATE = 2004,
        PRODUCT_DELETE = 2005,
        PRODUCT_UPDATE_STOCK = 2006,
        PRODUCT_MANAGE_PRICE = 2007,
        PRODUCT_MANAGE_QUANTITY = 2008,
        PRODUCT_MANAGE_DISCOUNT = 2009,
        PRODUCT_APPLY_CATEGORY_DISCOUNT = 2010,

        // 购物车相关
        CART_GET = 3000,
        CART_ADD_ITEM = 3001,
        CART_UPDATE_ITEM = 3002,
        CART_REMOVE_ITEM = 3003,
        CART_CLEAR = 3004, // 订单相关
        ORDER_CREATE = 4000,
        ORDER_DIRECT_PURCHASE = 4001,
        ORDER_GET_BY_USER = 4002,
        ORDER_GET_BY_ID = 4003,
        ORDER_UPDATE_STATUS = 4004,
        ORDER_GET_ALL = 4005,

        // 响应
        RESPONSE_SUCCESS = 5000,
        RESPONSE_ERROR = 5001,
        RESPONSE_DATA = 5002,

        // 心跳
        HEARTBEAT = 6000
    };

    // 用户类型
    enum class UserType : int
    {
        CUSTOMER = 1,
        SELLER = 2,
        ADMIN = 3
    };

    // 订单状态
    enum class OrderStatus : int
    {
        PENDING = 1,
        COMPLETED = 2,
        CANCELLED_STOCK = 3,
        CANCELLED_FUNDS = 4,
        CANCELLED_USER = 5
    };

    // 网络消息结构
    struct Message
    {
        MessageType type;
        std::string sessionId;                   // 会话ID
        std::map<std::string, std::string> data; // 消息数据

        Message() : type(MessageType::HEARTBEAT) {}
        Message(MessageType t, const std::string &sid = "") : type(t), sessionId(sid) {}

        // 序列化为字符串
        std::string serialize() const;

        // 从字符串反序列化
        static Message deserialize(const std::string &str);

        // 设置数据字段
        void setData(const std::string &key, const std::string &value)
        {
            data[key] = value;
        }

        // 获取数据字段
        std::string getData(const std::string &key) const
        {
            auto it = data.find(key);
            return (it != data.end()) ? it->second : "";
        }
    };

    // 用户数据结构
    struct UserData
    {
        std::string username;
        std::string password;
        UserType userType;
        double balance;

        std::string serialize() const;
        static UserData deserialize(const std::string &str);
    }; // 商品数据结构
    struct ProductData
    {
        std::string id;
        std::string name;
        std::string description;
        std::string type;     // book, clothing, food, generic
        double price;         // 折扣后的实际价格
        double originalPrice; // 原价
        int quantity;
        double discountRate;
        std::string sellerUsername;
        std::map<std::string, std::string> attributes; // 额外属性

        std::string serialize() const;
        static ProductData deserialize(const std::string &str);
    };

    // 购物车项数据结构
    struct CartItemData
    {
        std::string productId;
        std::string productName;
        int quantity;
        double priceAtAddition;
        std::string sellerUsername;

        std::string serialize() const;
        static CartItemData deserialize(const std::string &str);
    };

    // 订单数据结构
    struct OrderData
    {
        std::string orderId;
        std::string customerUsername;
        std::vector<CartItemData> items;
        double totalAmount;
        OrderStatus status;
        long long timestamp;

        std::string serialize() const;
        static OrderData deserialize(const std::string &str);
    };

} // namespace Protocol

#endif // PROTOCOL_H
