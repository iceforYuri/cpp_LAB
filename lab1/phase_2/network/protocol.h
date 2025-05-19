#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <map>
#include <vector>
#include "json.hpp"

// 使用nlohmann/json库进行JSON处理
using json = nlohmann::json;

// 请求类型
enum class RequestType
{
    LOGIN,          // 登录
    REGISTER,       // 注册
    PRODUCT_LIST,   // 获取产品列表
    PRODUCT_SEARCH, // 搜索产品
    PRODUCT_DETAIL, // 获取产品详情
    CART_ADD,       // 添加商品到购物车
    CART_REMOVE,    // 从购物车移除商品
    CART_UPDATE,    // 更新购物车商品数量
    CART_LIST,      // 获取购物车列表
    ORDER_CREATE,   // 创建订单
    ORDER_LIST,     // 获取订单列表
    ORDER_DETAIL,   // 获取订单详情
    ORDER_PAY,      // 支付订单
    ORDER_CANCEL,   // 取消订单
    ORDER_STATUS,   // 获取订单状态
    LOGOUT          // 登出
};

// 响应状态
enum class ResponseStatus
{
    SUCCESS,      // 成功
    FAILED,       // 失败
    UNAUTHORIZED, // 未授权
    NOT_FOUND,    // 未找到
    BAD_REQUEST,  // 错误的请求
    SERVER_ERROR  // 服务器错误
};

class Protocol
{
public:
    // 创建请求JSON
    static std::string createRequest(RequestType type, const json &data, const std::string &token = "");

    // 解析请求
    static bool parseRequest(const std::string &requestStr, RequestType &type, json &data, std::string &token);

    // 创建响应JSON
    static std::string createResponse(ResponseStatus status, const json &data, const std::string &message = "");

    // 解析响应
    static bool parseResponse(const std::string &responseStr, ResponseStatus &status, json &data, std::string &message);

    // 请求类型转字符串
    static std::string requestTypeToString(RequestType type);

    // 字符串转请求类型
    static RequestType stringToRequestType(const std::string &typeStr);

    // 响应状态转字符串
    static std::string responseStatusToString(ResponseStatus status);

    // 字符串转响应状态
    static ResponseStatus stringToResponseStatus(const std::string &statusStr);
};

#endif // PROTOCOL_H
