#include "request_handler.h"
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>

// Session实现
Session::Session(const std::string &token, const std::string &username)
    : token(token), username(username)
{
    updateActivity();
}

void Session::updateActivity()
{
    lastActivity = std::time(nullptr);
}

bool Session::isExpired(int timeoutSeconds) const
{
    time_t currentTime = std::time(nullptr);
    return (currentTime - lastActivity) > timeoutSeconds;
}

// RequestHandler实现
RequestHandler::RequestHandler(Store &store, OrderManager &orderManager, std::map<std::string, std::shared_ptr<User>> &users)
    : store(store), orderManager(orderManager), users(users)
{

    // 注册各种请求处理函数
    handlers[RequestType::LOGIN] = [this](const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
    {
        return this->handleLogin(data, clientIP, session);
    };

    handlers[RequestType::REGISTER] = [this](const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
    {
        return this->handleRegister(data, clientIP, session);
    };

    handlers[RequestType::LOGOUT] = [this](const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
    {
        return this->handleLogout(data, clientIP, session);
    };

    handlers[RequestType::PRODUCT_LIST] = [this](const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
    {
        return this->handleProductList(data, clientIP, session);
    };

    handlers[RequestType::PRODUCT_SEARCH] = [this](const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
    {
        return this->handleProductSearch(data, clientIP, session);
    };

    handlers[RequestType::PRODUCT_DETAIL] = [this](const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
    {
        return this->handleProductDetail(data, clientIP, session);
    };

    handlers[RequestType::CART_ADD] = [this](const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
    {
        return this->handleCartAdd(data, clientIP, session);
    };

    handlers[RequestType::CART_REMOVE] = [this](const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
    {
        return this->handleCartRemove(data, clientIP, session);
    };

    handlers[RequestType::CART_UPDATE] = [this](const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
    {
        return this->handleCartUpdate(data, clientIP, session);
    };

    handlers[RequestType::CART_LIST] = [this](const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
    {
        return this->handleCartList(data, clientIP, session);
    };

    handlers[RequestType::ORDER_CREATE] = [this](const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
    {
        return this->handleOrderCreate(data, clientIP, session);
    };

    handlers[RequestType::ORDER_LIST] = [this](const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
    {
        return this->handleOrderList(data, clientIP, session);
    };

    handlers[RequestType::ORDER_DETAIL] = [this](const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
    {
        return this->handleOrderDetail(data, clientIP, session);
    };

    handlers[RequestType::ORDER_PAY] = [this](const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
    {
        return this->handleOrderPay(data, clientIP, session);
    };

    handlers[RequestType::ORDER_CANCEL] = [this](const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
    {
        return this->handleOrderCancel(data, clientIP, session);
    };

    handlers[RequestType::ORDER_STATUS] = [this](const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
    {
        return this->handleOrderStatus(data, clientIP, session);
    };

    // 每小时清理一次过期会话
    std::thread([this]()
                {
        while (true) {
            std::this_thread::sleep_for(std::chrono::hours(1));
            this->cleanupExpiredSessions();
        } })
        .detach();
}

std::string RequestHandler::generateToken() const
{
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, chars.size() - 1);

    std::string token;
    for (int i = 0; i < 32; ++i)
    {
        token += chars[distribution(generator)];
    }

    return token;
}

std::shared_ptr<Session> RequestHandler::createSession(const std::string &username)
{
    std::string token = generateToken();
    auto session = std::make_shared<Session>(token, username);

    std::lock_guard<std::mutex> lock(sessionsMutex);
    sessions[token] = session;

    return session;
}

std::shared_ptr<Session> RequestHandler::getSession(const std::string &token)
{
    std::lock_guard<std::mutex> lock(sessionsMutex);

    auto it = sessions.find(token);
    if (it != sessions.end())
    {
        if (!it->second->isExpired())
        {
            it->second->updateActivity();
            return it->second;
        }
        else
        {
            sessions.erase(it);
        }
    }

    return nullptr;
}

void RequestHandler::removeSession(const std::string &token)
{
    std::lock_guard<std::mutex> lock(sessionsMutex);
    sessions.erase(token);
}

void RequestHandler::cleanupExpiredSessions()
{
    std::lock_guard<std::mutex> lock(sessionsMutex);

    for (auto it = sessions.begin(); it != sessions.end();)
    {
        if (it->second->isExpired())
        {
            it = sessions.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

std::string RequestHandler::handleRequest(const std::string &requestStr, const std::string &clientIP)
{
    RequestType type;
    json data;
    std::string token;

    // 解析请求
    if (!Protocol::parseRequest(requestStr, type, data, token))
    {
        return Protocol::createResponse(ResponseStatus::BAD_REQUEST, json({}), "Invalid request format");
    }

    // 检查是否需要认证
    std::shared_ptr<Session> session = nullptr;
    if (requiresAuthentication(type))
    {
        session = getSession(token);
        if (!session)
        {
            return Protocol::createResponse(ResponseStatus::UNAUTHORIZED, json({}), "Authentication required");
        }
    }

    // 找到对应的处理函数
    auto handlerIt = handlers.find(type);
    if (handlerIt == handlers.end())
    {
        return Protocol::createResponse(ResponseStatus::BAD_REQUEST, json({}), "Unsupported request type");
    }

    try
    {
        // 执行处理函数
        json responseData = handlerIt->second(data, clientIP, session);
        return Protocol::createResponse(ResponseStatus::SUCCESS, responseData);
    }
    catch (const std::exception &e)
    {
        return Protocol::createResponse(ResponseStatus::SERVER_ERROR, json({}), e.what());
    }
}

bool RequestHandler::requiresAuthentication(RequestType type) const
{
    // 这些请求类型不需要认证
    return type != RequestType::LOGIN && type != RequestType::REGISTER &&
           type != RequestType::PRODUCT_LIST && type != RequestType::PRODUCT_SEARCH &&
           type != RequestType::PRODUCT_DETAIL;
}

// 处理登录请求
json RequestHandler::handleLogin(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
{
    std::string username = data["username"];
    std::string password = data["password"];

    std::lock_guard<std::mutex> lock(usersMutex);

    auto it = users.find(username);
    if (it == users.end() || !it->second->checkPassword(password))
    {
        json response;
        response["success"] = false;
        response["message"] = "Invalid username or password";
        return response;
    }

    // 创建会话
    session = createSession(username);

    json response;
    response["success"] = true;
    response["token"] = session->getToken();
    response["userType"] = it->second->getUserType();
    response["username"] = username;

    return response;
}

// 处理注册请求
json RequestHandler::handleRegister(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
{
    std::string username = data["username"];
    std::string password = data["password"];
    std::string userType = data["userType"];

    std::lock_guard<std::mutex> lock(usersMutex);

    // 检查用户名是否已存在
    if (users.find(username) != users.end())
    {
        json response;
        response["success"] = false;
        response["message"] = "Username already exists";
        return response;
    }

    // 创建新用户
    std::shared_ptr<User> newUser;
    if (userType == "customer")
    {
        newUser = std::make_shared<Customer>(username, password);
    }
    else if (userType == "seller")
    {
        newUser = std::make_shared<Seller>(username, password);
    }
    else
    {
        json response;
        response["success"] = false;
        response["message"] = "Invalid user type";
        return response;
    }

    users[username] = newUser;

    // 保存用户数据
    saveUserData();

    // 创建会话
    session = createSession(username);

    json response;
    response["success"] = true;
    response["token"] = session->getToken();
    response["userType"] = userType;
    response["username"] = username;

    return response;
}

// 处理登出请求
json RequestHandler::handleLogout(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
{
    if (session)
    {
        removeSession(session->getToken());
    }

    json response;
    response["success"] = true;

    return response;
}

// 处理产品列表请求
json RequestHandler::handleProductList(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
{
    // 获取产品列表
    auto products = store.getProducts();

    json productList = json::array();
    for (const auto &product : products)
    {
        json productJson;
        productJson["id"] = product->getId();
        productJson["name"] = product->getName();
        productJson["price"] = product->getPrice();
        productJson["description"] = product->getDescription();
        productJson["seller"] = product->getSeller();

        productList.push_back(productJson);
    }

    json response;
    response["products"] = productList;

    return response;
}

// 处理产品搜索请求
json RequestHandler::handleProductSearch(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
{
    std::string keyword = data["keyword"];

    // 搜索产品
    auto products = store.searchProducts(keyword);

    json productList = json::array();
    for (const auto &product : products)
    {
        json productJson;
        productJson["id"] = product->getId();
        productJson["name"] = product->getName();
        productJson["price"] = product->getPrice();
        productJson["description"] = product->getDescription();
        productJson["seller"] = product->getSeller();

        productList.push_back(productJson);
    }

    json response;
    response["products"] = productList;

    return response;
}

// 处理产品详情请求
json RequestHandler::handleProductDetail(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
{
    int productId = data["productId"];

    // 获取产品详情
    auto product = store.getProduct(productId);
    if (!product)
    {
        json response;
        response["success"] = false;
        response["message"] = "Product not found";
        return response;
    }

    json productJson;
    productJson["id"] = product->getId();
    productJson["name"] = product->getName();
    productJson["price"] = product->getPrice();
    productJson["description"] = product->getDescription();
    productJson["seller"] = product->getSeller();
    // 可以添加更多产品详细信息

    json response;
    response["success"] = true;
    response["product"] = productJson;

    return response;
}

// 处理添加购物车请求
json RequestHandler::handleCartAdd(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
{
    int productId = data["productId"];
    int quantity = data["quantity"];

    // 获取用户
    std::lock_guard<std::mutex> lock(usersMutex);
    auto it = users.find(session->getUsername());
    if (it == users.end())
    {
        json response;
        response["success"] = false;
        response["message"] = "User not found";
        return response;
    }

    auto user = it->second;

    // 获取产品
    auto product = store.getProduct(productId);
    if (!product)
    {
        json response;
        response["success"] = false;
        response["message"] = "Product not found";
        return response;
    }

    // 添加到购物车
    if (user->getUserType() == "customer")
    {
        Customer *customer = dynamic_cast<Customer *>(user.get());
        if (customer)
        {
            customer->addToCart(product, quantity);
        }
    }

    json response;
    response["success"] = true;

    return response;
}

// 处理移除购物车请求
json RequestHandler::handleCartRemove(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
{
    int productId = data["productId"];

    // 获取用户
    std::lock_guard<std::mutex> lock(usersMutex);
    auto it = users.find(session->getUsername());
    if (it == users.end())
    {
        json response;
        response["success"] = false;
        response["message"] = "User not found";
        return response;
    }

    auto user = it->second;

    // 移除购物车项
    if (user->getUserType() == "customer")
    {
        Customer *customer = dynamic_cast<Customer *>(user.get());
        if (customer)
        {
            customer->removeFromCart(productId);
        }
    }

    json response;
    response["success"] = true;

    return response;
}

// 处理更新购物车请求
json RequestHandler::handleCartUpdate(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
{
    int productId = data["productId"];
    int quantity = data["quantity"];

    // 获取用户
    std::lock_guard<std::mutex> lock(usersMutex);
    auto it = users.find(session->getUsername());
    if (it == users.end())
    {
        json response;
        response["success"] = false;
        response["message"] = "User not found";
        return response;
    }

    auto user = it->second;

    // 更新购物车项
    if (user->getUserType() == "customer")
    {
        Customer *customer = dynamic_cast<Customer *>(user.get());
        if (customer)
        {
            customer->updateCartItemQuantity(productId, quantity);
        }
    }

    json response;
    response["success"] = true;

    return response;
}

// 处理购物车列表请求
json RequestHandler::handleCartList(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
{
    // 获取用户
    std::lock_guard<std::mutex> lock(usersMutex);
    auto it = users.find(session->getUsername());
    if (it == users.end())
    {
        json response;
        response["success"] = false;
        response["message"] = "User not found";
        return response;
    }

    auto user = it->second;

    json cartItems = json::array();

    // 获取购物车列表
    if (user->getUserType() == "customer")
    {
        Customer *customer = dynamic_cast<Customer *>(user.get());
        if (customer)
        {
            auto cart = customer->getCart();
            for (const auto &item : cart)
            {
                json cartItem;
                cartItem["productId"] = item.product->getId();
                cartItem["name"] = item.product->getName();
                cartItem["price"] = item.product->getPrice();
                cartItem["quantity"] = item.quantity;
                cartItem["totalPrice"] = item.product->getPrice() * item.quantity;

                cartItems.push_back(cartItem);
            }
        }
    }

    json response;
    response["items"] = cartItems;

    return response;
}

// 处理创建订单请求
json RequestHandler::handleOrderCreate(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
{
    // 获取用户
    std::lock_guard<std::mutex> lock(usersMutex);
    auto it = users.find(session->getUsername());
    if (it == users.end())
    {
        json response;
        response["success"] = false;
        response["message"] = "User not found";
        return response;
    }

    auto user = it->second;

    // 创建订单
    if (user->getUserType() == "customer")
    {
        Customer *customer = dynamic_cast<Customer *>(user.get());
        if (customer)
        {
            auto cart = customer->getCart();
            if (cart.empty())
            {
                json response;
                response["success"] = false;
                response["message"] = "Cart is empty";
                return response;
            }

            // 创建订单
            std::shared_ptr<Order> order = customer->createOrder();

            if (order)
            {
                // 将订单添加到订单管理器
                orderManager.addOrder(order);

                json response;
                response["success"] = true;
                response["orderId"] = order->getId();
                return response;
            }
        }
    }

    json response;
    response["success"] = false;
    response["message"] = "Failed to create order";

    return response;
}

// 处理订单列表请求
json RequestHandler::handleOrderList(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
{
    // 获取用户
    std::lock_guard<std::mutex> lock(usersMutex);
    auto it = users.find(session->getUsername());
    if (it == users.end())
    {
        json response;
        response["success"] = false;
        response["message"] = "User not found";
        return response;
    }

    auto user = it->second;

    json orderList = json::array();

    // 获取订单列表
    if (user->getUserType() == "customer")
    {
        Customer *customer = dynamic_cast<Customer *>(user.get());
        if (customer)
        {
            auto orders = customer->getOrders();
            for (const auto &order : orders)
            {
                json orderJson;
                orderJson["id"] = order->getId();
                orderJson["date"] = order->getDate();
                orderJson["status"] = order->getStatus();
                orderJson["totalAmount"] = order->getTotalAmount();

                orderList.push_back(orderJson);
            }
        }
    }
    else if (user->getUserType() == "seller")
    {
        Seller *seller = dynamic_cast<Seller *>(user.get());
        if (seller)
        {
            auto orders = seller->getOrders();
            for (const auto &order : orders)
            {
                json orderJson;
                orderJson["id"] = order->getId();
                orderJson["date"] = order->getDate();
                orderJson["status"] = order->getStatus();
                orderJson["totalAmount"] = order->getTotalAmount();
                orderJson["customer"] = order->getCustomer()->getUsername();

                orderList.push_back(orderJson);
            }
        }
    }

    json response;
    response["orders"] = orderList;

    return response;
}

// 处理订单详情请求
json RequestHandler::handleOrderDetail(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
{
    int orderId = data["orderId"];

    // 获取用户
    std::lock_guard<std::mutex> lock(usersMutex);
    auto it = users.find(session->getUsername());
    if (it == users.end())
    {
        json response;
        response["success"] = false;
        response["message"] = "User not found";
        return response;
    }

    auto user = it->second;

    // 获取订单
    std::shared_ptr<Order> order = nullptr;

    if (user->getUserType() == "customer")
    {
        Customer *customer = dynamic_cast<Customer *>(user.get());
        if (customer)
        {
            auto orders = customer->getOrders();
            for (const auto &o : orders)
            {
                if (o->getId() == orderId)
                {
                    order = o;
                    break;
                }
            }
        }
    }
    else if (user->getUserType() == "seller")
    {
        Seller *seller = dynamic_cast<Seller *>(user.get());
        if (seller)
        {
            auto orders = seller->getOrders();
            for (const auto &o : orders)
            {
                if (o->getId() == orderId)
                {
                    order = o;
                    break;
                }
            }
        }
    }

    if (!order)
    {
        json response;
        response["success"] = false;
        response["message"] = "Order not found";
        return response;
    }

    // 构建订单详情
    json orderJson;
    orderJson["id"] = order->getId();
    orderJson["date"] = order->getDate();
    orderJson["status"] = order->getStatus();
    orderJson["totalAmount"] = order->getTotalAmount();

    if (user->getUserType() == "seller")
    {
        orderJson["customer"] = order->getCustomer()->getUsername();
    }

    json itemsJson = json::array();
    auto items = order->getItems();
    for (const auto &item : items)
    {
        json itemJson;
        itemJson["productId"] = item.product->getId();
        itemJson["name"] = item.product->getName();
        itemJson["price"] = item.product->getPrice();
        itemJson["quantity"] = item.quantity;
        itemJson["totalPrice"] = item.product->getPrice() * item.quantity;

        itemsJson.push_back(itemJson);
    }

    orderJson["items"] = itemsJson;

    json response;
    response["success"] = true;
    response["order"] = orderJson;

    return response;
}

// 处理支付订单请求
json RequestHandler::handleOrderPay(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
{
    int orderId = data["orderId"];

    // 获取用户
    std::lock_guard<std::mutex> lock(usersMutex);
    auto it = users.find(session->getUsername());
    if (it == users.end())
    {
        json response;
        response["success"] = false;
        response["message"] = "User not found";
        return response;
    }

    auto user = it->second;

    // 获取订单
    std::shared_ptr<Order> order = nullptr;

    if (user->getUserType() == "customer")
    {
        Customer *customer = dynamic_cast<Customer *>(user.get());
        if (customer)
        {
            auto orders = customer->getOrders();
            for (const auto &o : orders)
            {
                if (o->getId() == orderId)
                {
                    order = o;
                    break;
                }
            }
        }
    }

    if (!order)
    {
        json response;
        response["success"] = false;
        response["message"] = "Order not found";
        return response;
    }

    // 支付订单
    order->setStatus("paid");

    json response;
    response["success"] = true;

    return response;
}

// 处理取消订单请求
json RequestHandler::handleOrderCancel(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
{
    int orderId = data["orderId"];

    // 获取用户
    std::lock_guard<std::mutex> lock(usersMutex);
    auto it = users.find(session->getUsername());
    if (it == users.end())
    {
        json response;
        response["success"] = false;
        response["message"] = "User not found";
        return response;
    }

    auto user = it->second;

    // 获取订单
    std::shared_ptr<Order> order = nullptr;

    if (user->getUserType() == "customer")
    {
        Customer *customer = dynamic_cast<Customer *>(user.get());
        if (customer)
        {
            auto orders = customer->getOrders();
            for (const auto &o : orders)
            {
                if (o->getId() == orderId)
                {
                    order = o;
                    break;
                }
            }
        }
    }

    if (!order)
    {
        json response;
        response["success"] = false;
        response["message"] = "Order not found";
        return response;
    }

    // 取消订单
    order->setStatus("canceled");

    json response;
    response["success"] = true;

    return response;
}

// 处理订单状态请求
json RequestHandler::handleOrderStatus(const json &data, const std::string &clientIP, std::shared_ptr<Session> &session)
{
    int orderId = data["orderId"];

    // 获取用户
    std::lock_guard<std::mutex> lock(usersMutex);
    auto it = users.find(session->getUsername());
    if (it == users.end())
    {
        json response;
        response["success"] = false;
        response["message"] = "User not found";
        return response;
    }

    auto user = it->second;

    // 获取订单
    std::shared_ptr<Order> order = nullptr;

    if (user->getUserType() == "customer")
    {
        Customer *customer = dynamic_cast<Customer *>(user.get());
        if (customer)
        {
            auto orders = customer->getOrders();
            for (const auto &o : orders)
            {
                if (o->getId() == orderId)
                {
                    order = o;
                    break;
                }
            }
        }
    }
    else if (user->getUserType() == "seller")
    {
        Seller *seller = dynamic_cast<Seller *>(user.get());
        if (seller)
        {
            auto orders = seller->getOrders();
            for (const auto &o : orders)
            {
                if (o->getId() == orderId)
                {
                    order = o;
                    break;
                }
            }
        }
    }

    if (!order)
    {
        json response;
        response["success"] = false;
        response["message"] = "Order not found";
        return response;
    }

    json response;
    response["success"] = true;
    response["status"] = order->getStatus();

    return response;
}

bool RequestHandler::loadUserData()
{
    // 从文件加载用户数据
    std::ifstream file("users.json");
    if (!file.is_open())
    {
        return false;
    }

    try
    {
        json usersData;
        file >> usersData;
        file.close();

        std::lock_guard<std::mutex> lock(usersMutex);
        users.clear();

        for (const auto &userData : usersData)
        {
            std::string username = userData["username"];
            std::string password = userData["password"];
            std::string userType = userData["userType"];

            if (userType == "customer")
            {
                users[username] = std::make_shared<Customer>(username, password);
            }
            else if (userType == "seller")
            {
                users[username] = std::make_shared<Seller>(username, password);
            }
        }

        return true;
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

bool RequestHandler::saveUserData()
{
    // 保存用户数据到文件
    json usersData = json::array();

    std::lock_guard<std::mutex> lock(usersMutex);
    for (const auto &userPair : users)
    {
        json userData;
        userData["username"] = userPair.first;
        userData["password"] = userPair.second->getPassword();
        userData["userType"] = userPair.second->getUserType();

        usersData.push_back(userData);
    }

    std::ofstream file("users.json");
    if (!file.is_open())
    {
        return false;
    }

    file << usersData.dump(4);
    file.close();

    return true;
}
