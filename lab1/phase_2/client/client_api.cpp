#include "client_api.h"
#include <iostream>

ClientAPI::ClientAPI(const std::string &serverHost, int serverPort)
    : socket(serverHost, serverPort), connected(false), authenticated(false)
{
}

ClientAPI::~ClientAPI()
{
    disconnect();
}

bool ClientAPI::connect()
{
    if (connected)
    {
        return true;
    }

    connected = socket.connectToServer();
    return connected;
}

void ClientAPI::disconnect()
{
    if (connected)
    {
        // 如果已认证，先尝试登出
        if (authenticated)
        {
            logout();
        }

        socket.close();
        connected = false;
        authenticated = false;
        token = "";
        username = "";
        userType = "";
    }
}

json ClientAPI::sendRequest(RequestType type, const json &data)
{
    if (!connected)
    {
        json error;
        error["success"] = false;
        error["message"] = "Not connected to server";
        return error;
    }

    // 创建请求JSON
    std::string requestStr = Protocol::createRequest(type, data, token);

    // 发送请求
    if (!socket.sendData(requestStr))
    {
        json error;
        error["success"] = false;
        error["message"] = "Failed to send request to server";
        return error;
    }

    // 接收响应
    std::string responseStr = socket.receiveData();
    if (responseStr.empty())
    {
        json error;
        error["success"] = false;
        error["message"] = "Empty response from server";
        return error;
    }

    // 解析响应
    ResponseStatus status;
    json responseData;
    std::string message;

    if (!Protocol::parseResponse(responseStr, status, responseData, message))
    {
        json error;
        error["success"] = false;
        error["message"] = "Failed to parse server response";
        return error;
    }

    // 如果状态不是成功，返回错误
    if (status != ResponseStatus::SUCCESS)
    {
        json error;
        error["success"] = false;
        error["message"] = message.empty() ? Protocol::responseStatusToString(status) : message;
        return error;
    }

    return responseData;
}

bool ClientAPI::login(const std::string &username, const std::string &password)
{
    // 准备请求数据
    json data;
    data["username"] = username;
    data["password"] = password;

    // 发送登录请求
    json response = sendRequest(RequestType::LOGIN, data);

    if (response.contains("success") && response["success"].get<bool>())
    {
        // 登录成功，保存令牌和用户信息
        this->token = response["token"];
        this->username = response["username"];
        this->userType = response["userType"];
        this->authenticated = true;
        return true;
    }

    return false;
}

bool ClientAPI::registerUser(const std::string &username, const std::string &password, const std::string &userType)
{
    // 准备请求数据
    json data;
    data["username"] = username;
    data["password"] = password;
    data["userType"] = userType;

    // 发送注册请求
    json response = sendRequest(RequestType::REGISTER, data);

    if (response.contains("success") && response["success"].get<bool>())
    {
        // 注册成功，自动登录保存令牌和用户信息
        this->token = response["token"];
        this->username = response["username"];
        this->userType = response["userType"];
        this->authenticated = true;
        return true;
    }

    return false;
}

bool ClientAPI::logout()
{
    if (!authenticated)
    {
        return true; // 已经是登出状态
    }

    // 发送登出请求
    json response = sendRequest(RequestType::LOGOUT, json({}));

    // 无论成功与否，都清除客户端保存的认证信息
    this->token = "";
    this->username = "";
    this->userType = "";
    this->authenticated = false;

    return response.contains("success") && response["success"].get<bool>();
}

std::vector<ProductInfo> ClientAPI::getProducts()
{
    std::vector<ProductInfo> products;

    // 发送获取产品列表请求
    json response = sendRequest(RequestType::PRODUCT_LIST, json({}));

    if (response.contains("products") && response["products"].is_array())
    {
        for (const auto &productJson : response["products"])
        {
            ProductInfo product;
            product.id = productJson["id"];
            product.name = productJson["name"];
            product.price = productJson["price"];
            product.description = productJson["description"];
            product.seller = productJson["seller"];

            products.push_back(product);
        }
    }

    return products;
}

std::vector<ProductInfo> ClientAPI::searchProducts(const std::string &keyword)
{
    std::vector<ProductInfo> products;

    // 准备请求数据
    json data;
    data["keyword"] = keyword;

    // 发送搜索产品请求
    json response = sendRequest(RequestType::PRODUCT_SEARCH, data);

    if (response.contains("products") && response["products"].is_array())
    {
        for (const auto &productJson : response["products"])
        {
            ProductInfo product;
            product.id = productJson["id"];
            product.name = productJson["name"];
            product.price = productJson["price"];
            product.description = productJson["description"];
            product.seller = productJson["seller"];

            products.push_back(product);
        }
    }

    return products;
}

ProductInfo ClientAPI::getProductDetail(int productId)
{
    ProductInfo product;

    // 准备请求数据
    json data;
    data["productId"] = productId;

    // 发送获取产品详情请求
    json response = sendRequest(RequestType::PRODUCT_DETAIL, data);

    if (response.contains("success") && response["success"].get<bool>() && response.contains("product"))
    {
        const auto &productJson = response["product"];
        product.id = productJson["id"];
        product.name = productJson["name"];
        product.price = productJson["price"];
        product.description = productJson["description"];
        product.seller = productJson["seller"];
    }

    return product;
}

bool ClientAPI::addToCart(int productId, int quantity)
{
    if (!authenticated)
    {
        return false;
    }

    // 准备请求数据
    json data;
    data["productId"] = productId;
    data["quantity"] = quantity;

    // 发送添加到购物车请求
    json response = sendRequest(RequestType::CART_ADD, data);

    return response.contains("success") && response["success"].get<bool>();
}

bool ClientAPI::removeFromCart(int productId)
{
    if (!authenticated)
    {
        return false;
    }

    // 准备请求数据
    json data;
    data["productId"] = productId;

    // 发送从购物车移除请求
    json response = sendRequest(RequestType::CART_REMOVE, data);

    return response.contains("success") && response["success"].get<bool>();
}

bool ClientAPI::updateCartItemQuantity(int productId, int quantity)
{
    if (!authenticated)
    {
        return false;
    }

    // 准备请求数据
    json data;
    data["productId"] = productId;
    data["quantity"] = quantity;

    // 发送更新购物车项请求
    json response = sendRequest(RequestType::CART_UPDATE, data);

    return response.contains("success") && response["success"].get<bool>();
}

std::vector<CartItemInfo> ClientAPI::getCart()
{
    std::vector<CartItemInfo> items;

    if (!authenticated)
    {
        return items;
    }

    // 发送获取购物车列表请求
    json response = sendRequest(RequestType::CART_LIST, json({}));

    if (response.contains("items") && response["items"].is_array())
    {
        for (const auto &itemJson : response["items"])
        {
            CartItemInfo item;
            item.productId = itemJson["productId"];
            item.name = itemJson["name"];
            item.price = itemJson["price"];
            item.quantity = itemJson["quantity"];
            item.totalPrice = itemJson["totalPrice"];

            items.push_back(item);
        }
    }

    return items;
}

bool ClientAPI::createOrder(int &orderId)
{
    if (!authenticated)
    {
        return false;
    }

    // 发送创建订单请求
    json response = sendRequest(RequestType::ORDER_CREATE, json({}));

    if (response.contains("success") && response["success"].get<bool>() && response.contains("orderId"))
    {
        orderId = response["orderId"];
        return true;
    }

    return false;
}

std::vector<OrderInfo> ClientAPI::getOrders()
{
    std::vector<OrderInfo> orders;

    if (!authenticated)
    {
        return orders;
    }

    // 发送获取订单列表请求
    json response = sendRequest(RequestType::ORDER_LIST, json({}));

    if (response.contains("orders") && response["orders"].is_array())
    {
        for (const auto &orderJson : response["orders"])
        {
            OrderInfo order;
            order.id = orderJson["id"];
            order.date = orderJson["date"];
            order.status = orderJson["status"];
            order.totalAmount = orderJson["totalAmount"];

            if (orderJson.contains("customer"))
            {
                order.customer = orderJson["customer"];
            }

            orders.push_back(order);
        }
    }

    return orders;
}

OrderInfo ClientAPI::getOrderDetail(int orderId)
{
    OrderInfo order;

    if (!authenticated)
    {
        return order;
    }

    // 准备请求数据
    json data;
    data["orderId"] = orderId;

    // 发送获取订单详情请求
    json response = sendRequest(RequestType::ORDER_DETAIL, data);

    if (response.contains("success") && response["success"].get<bool>() && response.contains("order"))
    {
        const auto &orderJson = response["order"];
        order.id = orderJson["id"];
        order.date = orderJson["date"];
        order.status = orderJson["status"];
        order.totalAmount = orderJson["totalAmount"];

        if (orderJson.contains("customer"))
        {
            order.customer = orderJson["customer"];
        }

        if (orderJson.contains("items") && orderJson["items"].is_array())
        {
            for (const auto &itemJson : orderJson["items"])
            {
                CartItemInfo item;
                item.productId = itemJson["productId"];
                item.name = itemJson["name"];
                item.price = itemJson["price"];
                item.quantity = itemJson["quantity"];
                item.totalPrice = itemJson["totalPrice"];

                order.items.push_back(item);
            }
        }
    }

    return order;
}

bool ClientAPI::payOrder(int orderId)
{
    if (!authenticated)
    {
        return false;
    }

    // 准备请求数据
    json data;
    data["orderId"] = orderId;

    // 发送支付订单请求
    json response = sendRequest(RequestType::ORDER_PAY, data);

    return response.contains("success") && response["success"].get<bool>();
}

bool ClientAPI::cancelOrder(int orderId)
{
    if (!authenticated)
    {
        return false;
    }

    // 准备请求数据
    json data;
    data["orderId"] = orderId;

    // 发送取消订单请求
    json response = sendRequest(RequestType::ORDER_CANCEL, data);

    return response.contains("success") && response["success"].get<bool>();
}

std::string ClientAPI::getOrderStatus(int orderId)
{
    if (!authenticated)
    {
        return "";
    }

    // 准备请求数据
    json data;
    data["orderId"] = orderId;

    // 发送获取订单状态请求
    json response = sendRequest(RequestType::ORDER_STATUS, data);

    if (response.contains("success") && response["success"].get<bool>() && response.contains("status"))
    {
        return response["status"];
    }

    return "";
}
