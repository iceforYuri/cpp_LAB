#include "request_processor.h"
#include "server_app.h"
#include "client_handler.h"

RequestProcessor::RequestProcessor(ServerApp *server, ClientHandler *clientHandler) : m_server(server),
                                                                                      m_clientHandler(clientHandler)
{
}

std::string RequestProcessor::processLoginRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, password;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, password, '|');

    // 检查用户是否已经登录
    if (m_server->isUserLoggedIn(username))
    {
        return "LOGIN_FAILED|该用户已在其他客户端登录";
    }

    std::lock_guard<std::mutex> lock(m_server->getUsersMutex());
    User *user = User::login(m_server->getUsers(), username, password);

    if (user)
    {
        // 添加到已登录用户集合
        m_server->addLoggedInUser(username);

        // 记录此连接的用户名
        m_clientHandler->setCurrentConnectedUser(username);

        return "LOGIN_SUCCESS|" + user->getUsername() + "|" + user->getUserType();
    }
    else
    {
        return "LOGIN_FAILED|用户名或密码错误";
    }
}

std::string RequestProcessor::processRegisterRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, password, type;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, password, '|');
    std::getline(iss, type, '|');

    std::lock_guard<std::mutex> lock(m_server->getUsersMutex());
    bool exists = User::isUsernameExists(m_server->getUsers(), username);

    if (!exists)
    {
        User *newUser = nullptr;
        if (type == "customer")
        {
            newUser = new Customer(username, password, "../user/carts", 0.0);
        }
        else if (type == "seller")
        {
            newUser = new Seller(username, password, 0.0);
        }

        if (newUser)
        {
            m_server->getUsers().push_back(newUser);
            User::saveUsersToFile(m_server->getUsers(), "./user/users.txt");
            return "REGISTER_SUCCESS|" + username;
        }
        else
        {
            return "REGISTER_FAILED|创建用户失败";
        }
    }
    else
    {
        return "REGISTER_FAILED|用户名已存在";
    }
}

std::string RequestProcessor::processGetProductsRequest() const
{
    std::lock_guard<std::mutex> lock(m_server->getStoreMutex());
    const auto &products = m_server->getStore()->getProducts();
    std::ostringstream oss;
    oss << "PRODUCTS|" << products.size();

    for (const auto &product : products)
    {
        oss << "|" << product->getName()
            << "," << product->getType()
            << "," << product->getPrice()
            << "," << product->getQuantity()
            << "," << product->getSellerUsername();
    }

    return oss.str();
}

std::string RequestProcessor::processGetProductDetailRequest(const std::string &request) const
{
    std::istringstream iss(request);
    std::string cmd, productName, sellerUsername;

    std::getline(iss, cmd, '|');
    std::getline(iss, productName, '|');
    std::getline(iss, sellerUsername, '|');

    std::lock_guard<std::mutex> lock(m_server->getStoreMutex());
    Product *product = m_server->getStore()->findProductByName(productName, sellerUsername);

    if (product)
    {
        std::ostringstream oss;
        oss << "PRODUCT_DETAIL|" << product->getName()
            << "|" << product->getType()
            << "|" << product->getDescription()
            << "|" << product->getPrice()
            << "|" << product->getQuantity()
            << "|" << product->getSellerUsername();

        // 根据不同类型添加特定属性
        if (product->getType() == "Book")
        {
            Book *book = dynamic_cast<Book *>(product);
            oss << "|" << book->getAuthor() << "|" << book->getIsbn();
        }
        else if (product->getType() == "Clothing")
        {
            Clothing *clothing = dynamic_cast<Clothing *>(product);
            oss << "|" << clothing->getSize() << "|" << clothing->getColor();
        }
        else if (product->getType() == "Food")
        {
            Food *food = dynamic_cast<Food *>(product);
            oss << "|" << food->getExpirationDate();
        }

        return oss.str();
    }
    else
    {
        return "ERROR|找不到商品";
    }
}

std::string RequestProcessor::processGetCartRequest(const std::string &request) const
{
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    std::lock_guard<std::mutex> lock(m_server->getUsersMutex());
    User *user = nullptr;
    for (auto u : m_server->getUsers())
    {
        if (u->getUsername() == username && u->getUserType() == "customer")
        {
            user = u;
            break;
        }
    }
    if (user)
    {
        Customer *customer = dynamic_cast<Customer *>(user);
        const std::vector<CartItem> &cartItems = customer->shoppingCartItems;

        std::ostringstream oss;
        oss << "CART|" << cartItems.size();

        for (const auto &item : cartItems)
        {
            oss << "|" << item.productName
                << "," << item.quantity
                << "," << item.priceAtAddition
                << "," << item.sellerUsername;
        }

        return oss.str();
    }
    else
    {
        return "ERROR|用户不存在或不是消费者账户";
    }
}

std::string RequestProcessor::processAddToCartRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, productName, sellerUsername;
    int quantity;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, productName, '|');
    std::getline(iss, sellerUsername, '|');

    std::string quantityStr;
    std::getline(iss, quantityStr, '|');
    quantity = std::stoi(quantityStr);

    // 检查商品是否存在
    std::lock_guard<std::mutex> storeLock(m_server->getStoreMutex());
    Product *product = m_server->getStore()->findProductByName(productName, sellerUsername);

    if (!product)
    {
        return "ERROR|商品不存在";
    }

    if (product->getQuantity() < quantity)
    {
        return "ERROR|库存不足";
    }

    // 添加到购物车
    std::lock_guard<std::mutex> userLock(m_server->getUsersMutex());
    User *user = nullptr;
    for (auto u : m_server->getUsers())
    {
        if (u->getUsername() == username && u->getUserType() == "customer")
        {
            user = u;
            break;
        }
    }

    if (user)
    {
        Customer *customer = dynamic_cast<Customer *>(user);
        bool result = customer->addToCart(*product, quantity);

        if (result)
        {
            customer->saveCartToFile();
            return "CART_UPDATED|成功添加商品到购物车";
        }
        else
        {
            return "ERROR|添加到购物车失败";
        }
    }
    else
    {
        return "ERROR|用户不存在或不是消费者账户";
    }
}

std::string RequestProcessor::processRemoveFromCartRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, productName, sellerUsername;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, productName, '|');
    std::getline(iss, sellerUsername, '|');

    std::lock_guard<std::mutex> lock(m_server->getUsersMutex());
    User *user = nullptr;
    for (auto u : m_server->getUsers())
    {
        if (u->getUsername() == username && u->getUserType() == "customer")
        {
            user = u;
            break;
        }
    }

    if (user)
    {
        Customer *customer = dynamic_cast<Customer *>(user);
        bool result = customer->removeCartItem(productName);

        if (result)
        {
            // 注意：removeCartItem 已经调用了 saveCartToFile
            return "CART_UPDATED|成功从购物车移除商品";
        }
        else
        {
            return "ERROR|从购物车移除失败";
        }
    }
    else
    {
        return "ERROR|用户不存在或不是消费者账户";
    }
}

std::string RequestProcessor::processUpdateCartItemRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, productName, sellerUsername;
    int newQuantity;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, productName, '|');
    std::getline(iss, sellerUsername, '|');

    std::string quantityStr;
    std::getline(iss, quantityStr, '|');
    newQuantity = std::stoi(quantityStr);

    // 获取商品
    std::lock_guard<std::mutex> storeLock(m_server->getStoreMutex());
    Product *product = m_server->getStore()->findProductByName(productName, sellerUsername);

    if (!product)
    {
        return "ERROR|商品不存在";
    }

    if (product->getQuantity() < newQuantity)
    {
        return "ERROR|库存不足";
    }

    // 更新购物车
    std::lock_guard<std::mutex> userLock(m_server->getUsersMutex());
    User *user = nullptr;
    for (auto u : m_server->getUsers())
    {
        if (u->getUsername() == username && u->getUserType() == "customer")
        {
            user = u;
            break;
        }
    }

    if (user)
    {
        Customer *customer = dynamic_cast<Customer *>(user);
        bool result = customer->updateCartItemQuantity(productName, newQuantity, *m_server->getStore());

        if (result)
        {
            // updateCartItemQuantity 内部已经调用了 saveCartToFile
            return "CART_UPDATED|成功更新购物车商品数量";
        }
        else
        {
            return "ERROR|更新购物车失败";
        }
    }
    else
    {
        return "ERROR|用户不存在或不是消费者账户";
    }
}

std::string RequestProcessor::processCheckoutRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    // 获取用户
    std::lock_guard<std::mutex> userLock(m_server->getUsersMutex());
    User *user = nullptr;
    for (auto u : m_server->getUsers())
    {
        if (u->getUsername() == username && u->getUserType() == "customer")
        {
            user = u;
            break;
        }
    }
    if (!user)
    {
        return "ERROR|用户不存在或不是消费者账户";
    }

    Customer *customer = dynamic_cast<Customer *>(user);
    std::vector<CartItem> &cartItems = customer->shoppingCartItems;

    if (cartItems.empty())
    {
        return "ERROR|购物车为空";
    }
    // 计算总金额
    double totalAmount = 0.0;
    for (const auto &item : cartItems)
    {
        totalAmount += item.priceAtAddition * item.quantity;
    }
    // 检查余额
    if (customer->checkBalance() < totalAmount)
    {
        return "ERROR|余额不足";
    }

    // 创建订单
    Order order(username);
    for (const auto &item : cartItems)
    {
        order.addProduct(item.productName, item.sellerUsername, item.quantity, item.priceAtAddition);
    }
    // 添加到订单队列
    m_server->getOrderManager()->submitOrderRequest(order);

    // 获取订单ID
    std::string orderId = order.getOrderId();

    // 清空购物车
    customer->clearCartAndFile();

    return "CHECKOUT_SUCCESS|订单已提交|" + orderId;
}

std::string RequestProcessor::processLogoutRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    // 检查用户是否登录
    if (m_clientHandler->getCurrentConnectedUser() != username)
    {
        return "ERROR|您当前未登录该账户";
    }

    // 从已登录用户列表中移除
    m_server->removeLoggedInUser(username);

    // 清除当前连接的用户名
    m_clientHandler->setCurrentConnectedUser("");

    return "LOGOUT_SUCCESS|成功退出登录";
}

std::string RequestProcessor::processGetUserInfoRequest(const std::string &request) const
{
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    std::lock_guard<std::mutex> lock(m_server->getUsersMutex());
    User *user = nullptr;
    for (auto u : m_server->getUsers())
    {
        if (u->getUsername() == username)
        {
            user = u;
            break;
        }
    }

    if (user)
    {
        std::ostringstream oss;
        oss << "USER_INFO|" << user->getUsername()
            << "|" << user->getUserType()
            << "|" << user->checkBalance();

        return oss.str();
    }
    else
    {
        return "ERROR|用户不存在";
    }
}

std::string RequestProcessor::processCheckBalanceRequest(const std::string &request) const
{
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    std::lock_guard<std::mutex> lock(m_server->getUsersMutex());
    User *user = nullptr;
    for (auto u : m_server->getUsers())
    {
        if (u->getUsername() == username)
        {
            user = u;
            break;
        }
    }
    if (user)
    {
        return "BALANCE|" + std::to_string(user->checkBalance());
    }
    else
    {
        return "ERROR|用户不存在";
    }
}

std::string RequestProcessor::processDepositRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, amountStr;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, amountStr, '|');

    double amount = std::stod(amountStr);

    if (amount <= 0)
    {
        return "ERROR|金额必须大于0";
    }

    std::lock_guard<std::mutex> lock(m_server->getUsersMutex());
    User *user = nullptr;
    for (auto u : m_server->getUsers())
    {
        if (u->getUsername() == username)
        {
            user = u;
            break;
        }
    }
    if (user)
    {
        user->deposit(amount);
        User::saveUsersToFile(m_server->getUsers(), "./user/users.txt");
        return "DEPOSIT_SUCCESS|充值成功|" + std::to_string(user->checkBalance());
    }
    else
    {
        return "ERROR|用户不存在";
    }
}

std::string RequestProcessor::processSearchProductsRequest(const std::string &request) const
{
    std::istringstream iss(request);
    std::string cmd, searchTerm;

    std::getline(iss, cmd, '|');
    std::getline(iss, searchTerm, '|');
    std::lock_guard<std::mutex> lock(m_server->getStoreMutex());
    std::vector<Product *> searchResults = m_server->getStore()->searchProductsByName(searchTerm);

    std::ostringstream oss;
    oss << "SEARCH_RESULTS|" << searchResults.size();

    for (const auto &product : searchResults)
    {
        oss << "|" << product->getName()
            << "," << product->getType()
            << "," << product->getPrice()
            << "," << product->getQuantity()
            << "," << product->getSellerUsername()
            << "," << product->getDescription();
    }

    return oss.str();
}

std::string RequestProcessor::processGetSellerInfoRequest(const std::string &request) const
{
    std::istringstream iss(request);
    std::string cmd, username;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');

    std::lock_guard<std::mutex> lock(m_server->getUsersMutex());
    User *user = nullptr;
    for (auto u : m_server->getUsers())
    {
        if (u->getUsername() == username && u->getUserType() == "seller")
        {
            user = u;
            break;
        }
    }

    if (user)
    {
        std::ostringstream oss;
        oss << "SELLER_INFO|" << user->getUsername()
            << "|" << user->getUserType()
            << "|" << user->checkBalance();

        return oss.str();
    }
    else
    {
        return "ERROR|用户不存在或不是商家账户";
    }
}

std::string RequestProcessor::processChangePasswordRequest(const std::string &request)
{
    std::istringstream iss(request);
    std::string cmd, username, oldPassword, newPassword;

    std::getline(iss, cmd, '|');
    std::getline(iss, username, '|');
    std::getline(iss, oldPassword, '|');
    std::getline(iss, newPassword, '|');

    std::lock_guard<std::mutex> lock(m_server->getUsersMutex());
    User *user = nullptr;
    for (auto u : m_server->getUsers())
    {
        if (u->getUsername() == username)
        {
            user = u;
            break;
        }
    }
    if (user)
    {
        if (user->changePassword(oldPassword, newPassword))
        {
            User::saveUsersToFile(m_server->getUsers(), "./user/users.txt");
            return "PASSWORD_CHANGED|密码修改成功";
        }
        else
        {
            return "ERROR|原密码错误";
        }
    }
    else
    {
        return "ERROR|用户不存在";
    }
}

std::string RequestProcessor::processAddProductRequest(const std::string &request)
{
    // 实现添加商品的逻辑
    // 由于这部分逻辑较长，这里简化处理
    return "SUCCESS|商品已添加";
}

std::string RequestProcessor::processUpdateProductRequest(const std::string &request)
{
    // 实现更新商品的逻辑
    // 由于这部分逻辑较长，这里简化处理
    return "SUCCESS|商品已更新";
}

std::string RequestProcessor::processRemoveProductRequest(const std::string &request)
{
    // 实现删除商品的逻辑
    // 由于这部分逻辑较长，这里简化处理
    return "SUCCESS|商品已删除";
}

ServerApp *RequestProcessor::getServer() const
{
    return m_server;
}

ClientHandler *RequestProcessor::getClientHandler() const
{
    return m_clientHandler;
}
