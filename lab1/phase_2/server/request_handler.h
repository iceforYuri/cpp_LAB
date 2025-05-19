#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <string>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <memory>
#include <vector>
#include "../network/json.hpp"
#include "../network/protocol.h"
#include "../user/user.h"
#include "../store/store.h"
#include "../order/order.h"
#include "../ordermanager/ordermanager.h"

using json = nlohmann::json;

class Session
{
private:
    std::string token;
    std::string username;
    time_t lastActivity;

public:
    Session(const std::string &token, const std::string &username);

    std::string getToken() const { return token; }
    std::string getUsername() const { return username; }
    time_t getLastActivity() const { return lastActivity; }

    void updateActivity();
    bool isExpired(int timeoutSeconds = 1800) const; // 默认30分钟超时
};

class RequestHandler
{
private:
    std::map<std::string, std::shared_ptr<Session>> sessions;
    std::mutex sessionsMutex;

    // 存储所有注册的请求处理函数
    std::map<RequestType, std::function<json(const json &, const std::string &, std::shared_ptr<Session> &)>> handlers;

    // 应用程序引用
    Store &store;
    OrderManager &orderManager;
    std::map<std::string, std::shared_ptr<User>> users;
    std::mutex usersMutex;

    // 生成唯一的会话令牌
    std::string generateToken() const;

    // 会话管理
    std::shared_ptr<Session> createSession(const std::string &username);
    std::shared_ptr<Session> getSession(const std::string &token);
    void removeSession(const std::string &token);
    void cleanupExpiredSessions();

    // 用户相关的请求处理器
    json handleLogin(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session);
    json handleRegister(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session);
    json handleLogout(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session);

    // 产品相关的请求处理器
    json handleProductList(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session);
    json handleProductSearch(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session);
    json handleProductDetail(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session);

    // 购物车相关的请求处理器
    json handleCartAdd(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session);
    json handleCartRemove(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session);
    json handleCartUpdate(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session);
    json handleCartList(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session);

    // 订单相关的请求处理器
    json handleOrderCreate(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session);
    json handleOrderList(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session);
    json handleOrderDetail(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session);
    json handleOrderPay(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session);
    json handleOrderCancel(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session);
    json handleOrderStatus(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session);

public:
    RequestHandler(Store &store, OrderManager &orderManager, std::map<std::string, std::shared_ptr<User>> &users);

    // 主处理方法，处理来自客户端的请求
    std::string handleRequest(const std::string &requestStr, const std::string &clientIP);

    // 需要认证的请求类型
    bool requiresAuthentication(RequestType type) const;

    // 加载用户数据
    bool loadUserData();

    // 保存用户数据
    bool saveUserData();
};

#endif // REQUEST_HANDLER_H
