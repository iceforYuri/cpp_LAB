#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>
#include <set>
#include <algorithm>
#include <chrono>

#include "user/user.h"
#include "store/store.h"
#include "order/order.h"
#include "ordermanager/ordermanager.h"

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// 全局变量
std::mutex g_usersMutex;
std::mutex g_storeMutex;
std::mutex g_loggedInUsersMutex; // 用于保护已登录用户列表的互斥量
std::atomic<bool> g_serverRunning(true);
std::set<std::string> g_loggedInUsers; // 存储已登录的用户名

// 文件路径常量
extern const string USER_FILE = "./user/users.txt";
extern const string STORE_FILE = "./store";
extern const string CART_FILE = "./user/carts";
extern const string ORDER_DIR = "./order/orders";

OrderManager g_orderManager(ORDER_DIR); // 创建订单管理器对象

// 处理客户端请求的函数
void handleClient(SOCKET clientSocket, vector<User *> &users, Store &store, OrderManager &orderManager)
{
    char buffer[4096] = {0};
    std::string response;
    bool clientConnected = true;
    std::string currentConnectedUser = ""; // 跟踪当前连接的用户名

    std::cout << "客户端已连接，处理中..." << std::endl;

    while (clientConnected && g_serverRunning)
    {
        // 清空缓冲区
        memset(buffer, 0, sizeof(buffer));

        // 接收客户端请求
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0)
        {
            std::cout << "客户端断开连接或接收错误" << std::endl;
            break;
        }

        // 解析请求
        std::string request(buffer);
        std::cout << "收到请求: " << request << std::endl;

        // 处理请求的核心逻辑
        std::string command = request.substr(0, request.find("|"));
        response = "ERROR|未知命令";
        if (command == "LOGIN")
        {
            // 登录请求: LOGIN|username|password
            std::istringstream iss(request);
            std::string cmd, username, password;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            std::getline(iss, password, '|');

            // 检查用户是否已经登录
            g_loggedInUsersMutex.lock();
            bool alreadyLoggedIn = (g_loggedInUsers.find(username) != g_loggedInUsers.end());
            g_loggedInUsersMutex.unlock();

            if (alreadyLoggedIn)
            {
                response = "LOGIN_FAILED|该用户已在其他客户端登录";
            }
            else
            {
                g_usersMutex.lock();
                User *user = User::login(users, username, password);
                g_usersMutex.unlock();

                if (user)
                { // 添加到已登录用户集合
                    g_loggedInUsersMutex.lock();
                    g_loggedInUsers.insert(username);
                    g_loggedInUsersMutex.unlock();

                    // 记录此连接的用户名
                    currentConnectedUser = username;

                    response = "LOGIN_SUCCESS|" + user->getUsername() + "|" + user->getUserType();
                }
                else
                {
                    response = "LOGIN_FAILED|用户名或密码错误";
                }
            }
        }
        else if (command == "REGISTER")
        {
            // 注册请求: REGISTER|username|password|type
            std::istringstream iss(request);
            std::string cmd, username, password, type;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            std::getline(iss, password, '|');
            std::getline(iss, type, '|');

            g_usersMutex.lock();
            bool exists = User::isUsernameExists(users, username);

            if (!exists)
            {
                User *newUser = nullptr;
                if (type == "customer")
                {
                    newUser = new Customer(username, password, CART_FILE, 0.0);
                }
                else if (type == "seller")
                {
                    newUser = new Seller(username, password, 0.0);
                }

                if (newUser)
                {
                    users.push_back(newUser);
                    User::saveUsersToFile(users, USER_FILE);
                    response = "REGISTER_SUCCESS|" + username;
                }
                else
                {
                    response = "REGISTER_FAILED|创建用户失败";
                }
            }
            else
            {
                response = "REGISTER_FAILED|用户名已存在";
            }
            g_usersMutex.unlock();
        }
        else if (command == "GET_PRODUCTS")
        {
            // 获取商品列表: GET_PRODUCTS
            g_storeMutex.lock();
            const auto &products = store.getProducts();
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
            g_storeMutex.unlock();

            response = oss.str();
        }
        else if (command == "GET_PRODUCT_DETAIL")
        {
            // 获取商品详情: GET_PRODUCT_DETAIL|productName|sellerUsername
            std::istringstream iss(request);
            std::string cmd, productName, sellerUsername;

            std::getline(iss, cmd, '|');
            std::getline(iss, productName, '|');
            std::getline(iss, sellerUsername, '|');

            g_storeMutex.lock();
            Product *product = store.findProductByName(productName, sellerUsername);

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

                response = oss.str();
            }
            else
            {
                response = "PRODUCT_NOT_FOUND|产品不存在";
            }
            g_storeMutex.unlock();
        }
        else if (command == "ADD_TO_CART")
        {
            // 添加到购物车: ADD_TO_CART|username|productName|sellerUsername|quantity
            std::istringstream iss(request);
            std::string cmd, username, productName, sellerUsername;
            int quantity;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            std::getline(iss, productName, '|');
            std::getline(iss, sellerUsername, '|');
            iss >> quantity;

            g_usersMutex.lock();
            g_storeMutex.lock();

            User *user = User::findUser(users, username);
            Customer *customer = dynamic_cast<Customer *>(user);
            Product *product = store.findProductByName(productName, sellerUsername);

            if (customer && product)
            {
                if (product->getQuantity() < quantity)
                {
                    response = "CART_ERROR|库存不足，当前库存: " + std::to_string(product->getQuantity());
                }
                else
                {
                    // 直接操作 Customer 的 shoppingCartItems
                    bool found = false;
                    customer->loadCartFromFile(); // 确保购物车是最新的

                    for (auto &item : customer->shoppingCartItems)
                    {
                        if (item.productName == productName && item.sellerUsername == sellerUsername)
                        {
                            item.quantity += quantity;
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        // 添加新商品到购物车
                        CartItem newItem;
                        newItem.productId = product->getName(); // 为了兼容性，保留 productId
                        newItem.productName = product->getName();
                        newItem.sellerUsername = sellerUsername;
                        newItem.quantity = quantity;
                        newItem.priceAtAddition = product->getPrice();
                        customer->shoppingCartItems.push_back(newItem);
                    }

                    customer->saveCartToFile();
                    response = "CART_UPDATED|添加成功";
                }
            }
            else
            {
                response = "CART_ERROR|用户或产品不存在";
            }

            g_storeMutex.unlock();
            g_usersMutex.unlock();
        }
        else if (command == "GET_CART")
        {
            // 获取购物车: GET_CART|username
            std::istringstream iss(request);
            std::string cmd, username;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');

            g_usersMutex.lock();
            User *user = User::findUser(users, username);
            Customer *customer = dynamic_cast<Customer *>(user);

            if (customer)
            {
                customer->loadCartFromFile();
                std::ostringstream oss;
                oss << "CART|" << customer->shoppingCartItems.size();

                for (const auto &item : customer->shoppingCartItems)
                {
                    oss << "|" << item.productName
                        << "," << item.quantity
                        << "," << item.priceAtAddition
                        << "," << item.sellerUsername;
                }

                response = oss.str();
            }
            else
            {
                response = "CART_ERROR|用户不存在或不是消费者";
            }
            g_usersMutex.unlock();
        }
        else if (command == "CHECKOUT")
        {
            // 结算购物车: CHECKOUT|username
            std::istringstream iss(request);
            std::string cmd, username;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');

            g_usersMutex.lock();
            g_storeMutex.lock();

            User *user = User::findUser(users, username);
            Customer *customer = dynamic_cast<Customer *>(user);

            if (customer)
            {
                customer->loadCartFromFile();

                if (customer->isCartEmpty())
                {
                    response = "CHECKOUT_ERROR|购物车为空";
                }
                else
                {
                    Order order(username);
                    double totalAmount = 0.0;
                    bool allItemsValid = true;

                    for (const auto &item : customer->shoppingCartItems)
                    {
                        Product *product = store.findProductByName(item.productName, item.sellerUsername);
                        if (product && product->getQuantity() >= item.quantity)
                        {
                            order.addItemFromProduct(*product, item.quantity);
                            totalAmount += item.quantity * product->getPrice();
                        }
                        else
                        {
                            allItemsValid = false;
                            break;
                        }
                    }

                    if (allItemsValid && customer->checkBalance() >= totalAmount)
                    {
                        customer->withdraw(totalAmount);
                        order.calculateTotalAmount();
                        auto orderPtr = orderManager.submitOrderRequest(order);

                        // 等待订单处理完成
                        auto startTime = std::chrono::steady_clock::now();
                        bool orderProcessed = false;

                        // 最多等待10秒
                        while (!orderProcessed &&
                               std::chrono::duration_cast<std::chrono::seconds>(
                                   std::chrono::steady_clock::now() - startTime)
                                       .count() < 10)
                        {
                            if (orderPtr->getProcessed())
                            {
                                orderProcessed = true;
                                break;
                            }

                            // 处理订单并更新库存（如果有订单在队列中）
                            orderManager.processNextOrder(store, users);

                            // 短暂暂停，避免CPU过度使用
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }

                        if (orderProcessed)
                        {
                            if (orderPtr->getStatus() == "COMPLETED" ||
                                orderPtr->getStatus() == "COMPLETED_WITH_PAYMENT_ISSUES")
                            {
                                // 清空购物车
                                customer->clearCartAndFile();

                                // 保存用户数据
                                User::saveUsersToFile(users, USER_FILE);

                                response = "CHECKOUT_SUCCESS|订单已提交|" + orderPtr->getOrderId();
                            }
                            else
                            {
                                // 订单处理失败，返回错误信息
                                response = "CHECKOUT_ERROR|订单处理失败: " + orderPtr->getStatus();
                            }
                        }
                        else
                        {
                            // 订单处理超时，返回等待信息
                            response = "CHECKOUT_PENDING|订单正在处理中，请稍后查询|" + orderPtr->getOrderId();
                        }
                    }
                    else if (!allItemsValid)
                    {
                        response = "CHECKOUT_ERROR|部分商品库存不足";
                    }
                    else
                    {
                        response = "CHECKOUT_ERROR|余额不足";
                    }
                }
            }
            else
            {
                response = "CHECKOUT_ERROR|用户不存在或不是消费者";
            }

            g_storeMutex.unlock();
            g_usersMutex.unlock();
        }
        else if (command == "GET_SELLER_INFO")
        {
            // 获取商家信息: GET_SELLER_INFO|username
            std::istringstream iss(request);
            std::string cmd, username;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');

            g_usersMutex.lock();
            User *user = User::findUser(users, username);

            if (user && user->getUserType() == "seller")
            {
                std::ostringstream oss;
                oss << "SELLER_INFO|" << user->getUsername() << "|"
                    << user->getUserType() << "|" << user->checkBalance();

                response = oss.str();
            }
            else
            {
                response = "SELLER_ERROR|用户不存在或不是商家";
            }
            g_usersMutex.unlock();
        }
        else if (command == "CHANGE_PASSWORD")
        {
            // 修改密码: CHANGE_PASSWORD|username|oldPassword|newPassword
            std::istringstream iss(request);
            std::string cmd, username, oldPassword, newPassword;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            std::getline(iss, oldPassword, '|');
            std::getline(iss, newPassword, '|');

            g_usersMutex.lock();
            User *user = User::findUser(users, username);

            if (user)
            {
                if (user->changePassword(oldPassword, newPassword))
                {
                    User::saveUsersToFile(users, USER_FILE);
                    response = "PASSWORD_CHANGED|密码修改成功";
                }
                else
                {
                    response = "PASSWORD_ERROR|密码修改失败，可能原密码错误";
                }
            }
            else
            {
                response = "PASSWORD_ERROR|用户不存在";
            }
            g_usersMutex.unlock();
        }
        else if (command == "GET_SELLER_PRODUCTS")
        {
            // 获取商家商品: GET_SELLER_PRODUCTS|username
            std::istringstream iss(request);
            std::string cmd, username;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');

            g_storeMutex.lock();
            std::vector<Product *> sellerProducts = store.getSellerProducts(username);

            std::ostringstream oss;
            oss << "SELLER_PRODUCTS|" << sellerProducts.size();

            for (const auto &product : sellerProducts)
            {
                oss << "|" << product->getName()
                    << "," << product->getType()
                    << "," << product->getOriginalPrice()
                    << "," << product->getQuantity()
                    << "," << product->getDiscountRate();
            }
            g_storeMutex.unlock();

            response = oss.str();
        }
        else if (command == "ADD_BOOK")
        {
            // 添加书籍: ADD_BOOK|username|name|description|price|quantity|author|isbn
            std::istringstream iss(request);
            std::string cmd, username, name, description, priceStr, quantityStr, author, isbn;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            std::getline(iss, name, '|');
            std::getline(iss, description, '|');
            std::getline(iss, priceStr, '|');
            std::getline(iss, quantityStr, '|');
            std::getline(iss, author, '|');
            std::getline(iss, isbn, '|');

            double price = std::stod(priceStr);
            int quantity = std::stoi(quantityStr);

            g_usersMutex.lock();
            g_storeMutex.lock();

            User *user = User::findUser(users, username);

            if (user && user->getUserType() == "seller")
            {
                if (store.createBook(user, name, description, price, quantity, author, isbn))
                {
                    response = "PRODUCT_ADDED|书籍添加成功";
                }
                else
                {
                    response = "PRODUCT_ERROR|书籍添加失败，可能是名称重复";
                }
            }
            else
            {
                response = "PRODUCT_ERROR|用户不存在或不是商家";
            }

            g_storeMutex.unlock();
            g_usersMutex.unlock();
        }
        else if (command == "ADD_CLOTHING")
        {
            // 添加服装: ADD_CLOTHING|username|name|description|price|quantity|size|color
            std::istringstream iss(request);
            std::string cmd, username, name, description, priceStr, quantityStr, size, color;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            std::getline(iss, name, '|');
            std::getline(iss, description, '|');
            std::getline(iss, priceStr, '|');
            std::getline(iss, quantityStr, '|');
            std::getline(iss, size, '|');
            std::getline(iss, color, '|');

            double price = std::stod(priceStr);
            int quantity = std::stoi(quantityStr);

            g_usersMutex.lock();
            g_storeMutex.lock();

            User *user = User::findUser(users, username);

            if (user && user->getUserType() == "seller")
            {
                if (store.createClothing(user, name, description, price, quantity, size, color))
                {
                    response = "PRODUCT_ADDED|服装添加成功";
                }
                else
                {
                    response = "PRODUCT_ERROR|服装添加失败，可能是名称重复";
                }
            }
            else
            {
                response = "PRODUCT_ERROR|用户不存在或不是商家";
            }

            g_storeMutex.unlock();
            g_usersMutex.unlock();
        }
        else if (command == "ADD_FOOD")
        {
            // 添加食品: ADD_FOOD|username|name|description|price|quantity|expirationDate
            std::istringstream iss(request);
            std::string cmd, username, name, description, priceStr, quantityStr, expDate;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            std::getline(iss, name, '|');
            std::getline(iss, description, '|');
            std::getline(iss, priceStr, '|');
            std::getline(iss, quantityStr, '|');
            std::getline(iss, expDate, '|');

            double price = std::stod(priceStr);
            int quantity = std::stoi(quantityStr);

            g_usersMutex.lock();
            g_storeMutex.lock();

            User *user = User::findUser(users, username);

            if (user && user->getUserType() == "seller")
            {
                if (store.createFood(user, name, description, price, quantity, expDate))
                {
                    response = "PRODUCT_ADDED|食品添加成功";
                }
                else
                {
                    response = "PRODUCT_ERROR|食品添加失败，可能是名称重复";
                }
            }
            else
            {
                response = "PRODUCT_ERROR|用户不存在或不是商家";
            }

            g_storeMutex.unlock();
            g_usersMutex.unlock();
        }
        else if (command == "ADD_GENERIC")
        {
            // 添加通用商品: ADD_GENERIC|username|name|description|price|quantity|categoryTag
            std::istringstream iss(request);
            std::string cmd, username, name, description, priceStr, quantityStr, categoryTag;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            std::getline(iss, name, '|');
            std::getline(iss, description, '|');
            std::getline(iss, priceStr, '|');
            std::getline(iss, quantityStr, '|');
            std::getline(iss, categoryTag, '|');

            double price = std::stod(priceStr);
            int quantity = std::stoi(quantityStr);

            g_usersMutex.lock();
            g_storeMutex.lock();

            User *user = User::findUser(users, username);

            if (user && user->getUserType() == "seller")
            {
                if (store.createGenericProduct(user, name, description, price, quantity, categoryTag))
                {
                    response = "PRODUCT_ADDED|商品添加成功";
                }
                else
                {
                    response = "PRODUCT_ERROR|商品添加失败，可能是名称重复";
                }
            }
            else
            {
                response = "PRODUCT_ERROR|用户不存在或不是商家";
            }

            g_storeMutex.unlock();
            g_usersMutex.unlock();
        }
        else if (command == "UPDATE_PRODUCT_PRICE")
        {
            // 修改商品价格: UPDATE_PRODUCT_PRICE|username|productName|newPrice
            std::istringstream iss(request);
            std::string cmd, username, productName, priceStr;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            std::getline(iss, productName, '|');
            std::getline(iss, priceStr, '|');

            double newPrice = std::stod(priceStr);

            g_usersMutex.lock();
            g_storeMutex.lock();

            User *user = User::findUser(users, username);

            if (user && user->getUserType() == "seller")
            {
                if (store.manageProductPrice(user, productName, newPrice))
                {
                    response = "PRODUCT_UPDATED|价格修改成功";
                }
                else
                {
                    response = "PRODUCT_ERROR|价格修改失败，可能是商品不存在或不属于该商家";
                }
            }
            else
            {
                response = "PRODUCT_ERROR|用户不存在或不是商家";
            }

            g_storeMutex.unlock();
            g_usersMutex.unlock();
        }
        else if (command == "UPDATE_PRODUCT_QUANTITY")
        {
            // 修改商品库存: UPDATE_PRODUCT_QUANTITY|username|productName|newQuantity
            std::istringstream iss(request);
            std::string cmd, username, productName, quantityStr;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            std::getline(iss, productName, '|');
            std::getline(iss, quantityStr, '|');

            int newQuantity = std::stoi(quantityStr);

            g_usersMutex.lock();
            g_storeMutex.lock();

            User *user = User::findUser(users, username);

            if (user && user->getUserType() == "seller")
            {
                if (store.manageProductQuantity(user, productName, newQuantity))
                {
                    response = "PRODUCT_UPDATED|库存修改成功";
                }
                else
                {
                    response = "PRODUCT_ERROR|库存修改失败，可能是商品不存在或不属于该商家";
                }
            }
            else
            {
                response = "PRODUCT_ERROR|用户不存在或不是商家";
            }

            g_storeMutex.unlock();
            g_usersMutex.unlock();
        }
        else if (command == "SET_PRODUCT_DISCOUNT")
        {
            // 设置商品折扣: SET_PRODUCT_DISCOUNT|username|productName|discountRate
            std::istringstream iss(request);
            std::string cmd, username, productName, discountStr;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            std::getline(iss, productName, '|');
            std::getline(iss, discountStr, '|');

            double discount = std::stod(discountStr);

            g_usersMutex.lock();
            g_storeMutex.lock();

            User *user = User::findUser(users, username);

            if (user && user->getUserType() == "seller")
            {
                if (store.manageProductDiscount(user, productName, discount))
                {
                    response = "PRODUCT_UPDATED|折扣设置成功";
                }
                else
                {
                    response = "PRODUCT_ERROR|折扣设置失败，可能是商品不存在或不属于该商家";
                }
            }
            else
            {
                response = "PRODUCT_ERROR|用户不存在或不是商家";
            }

            g_storeMutex.unlock();
            g_usersMutex.unlock();
        }
        else if (command == "SET_CATEGORY_DISCOUNT")
        {
            // 设置分类折扣: SET_CATEGORY_DISCOUNT|username|category|discountRate
            std::istringstream iss(request);
            std::string cmd, username, category, discountStr;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            std::getline(iss, category, '|');
            std::getline(iss, discountStr, '|');

            double discount = std::stod(discountStr);

            g_usersMutex.lock();
            g_storeMutex.lock();

            User *user = User::findUser(users, username);

            if (user && user->getUserType() == "seller")
            {
                if (store.applyCategoryDiscount(user, category, discount))
                {
                    response = "CATEGORY_UPDATED|分类折扣设置成功";
                }
                else
                {
                    response = "PRODUCT_ERROR|分类折扣设置失败，可能是分类不存在或不包含该商家的商品";
                }
            }
            else
            {
                response = "PRODUCT_ERROR|用户不存在或不是商家";
            }

            g_storeMutex.unlock();
            g_usersMutex.unlock();
        }
        else if (command == "LOGOUT")
        {
            // 处理登出请求: LOGOUT|username
            std::istringstream iss(request);
            std::string cmd, username;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            g_loggedInUsersMutex.lock();
            auto it = g_loggedInUsers.find(username);
            if (it != g_loggedInUsers.end())
            {
                g_loggedInUsers.erase(it);
                // 清除当前连接的用户名
                if (currentConnectedUser == username)
                {
                    currentConnectedUser = "";
                }
                response = "LOGOUT_SUCCESS|用户已成功登出";
            }
            else
            {
                response = "LOGOUT_FAILED|用户未登录";
            }
            g_loggedInUsersMutex.unlock();
        }
        else if (command == "CHECK_BALANCE")
        {
            // 查询余额: CHECK_BALANCE|username
            std::istringstream iss(request);
            std::string cmd, username;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');

            g_usersMutex.lock();
            User *user = User::findUser(users, username);

            if (user)
            {
                double balance = user->checkBalance();
                response = "BALANCE|" + std::to_string(balance);
            }
            else
            {
                response = "BALANCE_ERROR|用户不存在";
            }
            g_usersMutex.unlock();
        }
        else if (command == "GET_USER_INFO")
        {
            // 获取用户信息: GET_USER_INFO|username
            std::istringstream iss(request);
            std::string cmd, username;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');

            g_usersMutex.lock();
            User *user = User::findUser(users, username);

            if (user)
            {
                std::ostringstream oss;
                oss << "USER_INFO|" << user->getUsername() << "|"
                    << user->getUserType() << "|" << user->checkBalance();

                response = oss.str();
            }
            else
            {
                response = "USER_ERROR|用户不存在";
            }
            g_usersMutex.unlock();
        }
        else if (command == "DEPOSIT")
        {
            // 充值: DEPOSIT|username|amount
            std::istringstream iss(request);
            std::string cmd, username, amountStr;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            std::getline(iss, amountStr, '|');

            double amount = std::stod(amountStr);

            g_usersMutex.lock();
            User *user = User::findUser(users, username);

            if (user)
            {
                if (user->deposit(amount))
                {
                    User::saveUsersToFile(users, USER_FILE);
                    response = "DEPOSIT_SUCCESS|充值成功|" + std::to_string(user->checkBalance());
                }
                else
                {
                    response = "DEPOSIT_FAILED|充值失败";
                }
            }
            else
            {
                response = "DEPOSIT_FAILED|用户不存在";
            }
            g_usersMutex.unlock();
        }
        else if (command == "SEARCH_PRODUCTS")
        {
            // 按名称搜索商品: SEARCH_PRODUCTS|searchTerm
            std::istringstream iss(request);
            std::string cmd, searchTerm;

            std::getline(iss, cmd, '|');
            std::getline(iss, searchTerm, '|');

            g_storeMutex.lock();

            std::vector<Product *> searchResults;
            const auto &allProducts = store.getProducts();

            // 简单的包含搜索
            for (const auto &product : allProducts)
            {
                if (product->getName().find(searchTerm) != std::string::npos)
                {
                    searchResults.push_back(product);
                }
            }

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

            g_storeMutex.unlock();
            response = oss.str();
        }
        else if (command == "DIRECT_PURCHASE")
        {
            // 直接购买商品: DIRECT_PURCHASE|username|productName|sellerUsername|quantity
            std::istringstream iss(request);
            std::string cmd, username, productName, sellerUsername, quantityStr;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            std::getline(iss, productName, '|');
            std::getline(iss, sellerUsername, '|');
            std::getline(iss, quantityStr, '|');

            int quantity = std::stoi(quantityStr);

            g_usersMutex.lock();
            g_storeMutex.lock();

            User *user = User::findUser(users, username);
            Customer *customer = dynamic_cast<Customer *>(user);
            Product *product = store.findProductByName(productName, sellerUsername);

            if (!customer || !product)
            {
                g_storeMutex.unlock();
                g_usersMutex.unlock();

                if (!customer)
                    response = "PURCHASE_FAILED|用户不存在或不是消费者";
                else
                    response = "PURCHASE_FAILED|商品不存在";

                return;
            }

            // 检查库存
            if (product->getQuantity() < quantity)
            {
                g_storeMutex.unlock();
                g_usersMutex.unlock();
                response = "PURCHASE_FAILED|商品库存不足";
                return;
            }

            // 检查余额
            double totalCost = product->getPrice() * quantity;
            if (customer->checkBalance() < totalCost)
            {
                g_storeMutex.unlock();
                g_usersMutex.unlock();
                response = "PURCHASE_FAILED|用户余额不足";
                return;
            }

            // 创建订单
            Order order(username);
            order.addItemFromProduct(*product, quantity);
            order.calculateTotalAmount();

            // 处理订单
            auto orderPtr = g_orderManager.submitOrderRequest(order);

            // 处理订单并更新库存、扣款等
            g_orderManager.processNextOrder(store, users);

            // 保存用户数据
            User::saveUsersToFile(users, USER_FILE);

            g_storeMutex.unlock();
            g_usersMutex.unlock();

            response = "PURCHASE_SUCCESS|购买成功|" + orderPtr->getOrderId();
        }
        else if (command == "UPDATE_CART_ITEM")
        {
            // 更新购物车商品数量: UPDATE_CART_ITEM|username|productName|sellerUsername|newQuantity
            std::istringstream iss(request);
            std::string cmd, username, productName, sellerUsername;
            int newQuantity;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            std::getline(iss, productName, '|');
            std::getline(iss, sellerUsername, '|');
            iss >> newQuantity;

            g_usersMutex.lock();
            g_storeMutex.lock();

            User *user = User::findUser(users, username);
            Customer *customer = dynamic_cast<Customer *>(user);

            if (customer)
            {
                bool updated = false;
                customer->loadCartFromFile();

                for (auto &item : customer->shoppingCartItems)
                {
                    if (item.productName == productName && item.sellerUsername == sellerUsername)
                    {
                        // 验证库存充足
                        Product *product = store.findProductByName(productName, sellerUsername);
                        if (product && product->getQuantity() >= newQuantity)
                        {
                            item.quantity = newQuantity;
                            updated = true;
                            break;
                        }
                        else
                        {
                            response = "CART_ERROR|库存不足，无法更新数量";
                            g_storeMutex.unlock();
                            g_usersMutex.unlock();
                            return;
                        }
                    }
                }

                if (updated)
                {
                    customer->saveCartToFile();
                    response = "CART_UPDATED|商品数量已更新";
                }
                else
                {
                    response = "CART_ERROR|购物车中未找到该商品";
                }
            }
            else
            {
                response = "CART_ERROR|用户不存在或不是消费者";
            }

            g_storeMutex.unlock();
            g_usersMutex.unlock();
        }
        else if (command == "REMOVE_FROM_CART")
        {
            // 从购物车移除商品: REMOVE_FROM_CART|username|productName|sellerUsername
            std::istringstream iss(request);
            std::string cmd, username, productName, sellerUsername;

            std::getline(iss, cmd, '|');
            std::getline(iss, username, '|');
            std::getline(iss, productName, '|');
            std::getline(iss, sellerUsername, '|');

            g_usersMutex.lock();
            User *user = User::findUser(users, username);
            Customer *customer = dynamic_cast<Customer *>(user);

            if (customer)
            {
                bool removed = false;
                customer->loadCartFromFile();

                auto it = std::remove_if(customer->shoppingCartItems.begin(), customer->shoppingCartItems.end(),
                                         [&productName, &sellerUsername](const CartItem &item)
                                         {
                                             return (item.productName == productName && item.sellerUsername == sellerUsername);
                                         });

                if (it != customer->shoppingCartItems.end())
                {
                    customer->shoppingCartItems.erase(it, customer->shoppingCartItems.end());
                    customer->saveCartToFile();
                    response = "CART_UPDATED|商品已从购物车移除";
                    removed = true;
                }

                if (!removed)
                {
                    response = "CART_ERROR|购物车中未找到该商品";
                }
            }
            else
            {
                response = "CART_ERROR|用户不存在或不是消费者";
            }
            g_usersMutex.unlock();
        }

        // 发送响应给客户端
        send(clientSocket, response.c_str(), response.size(), 0);
    }

    // 检查此连接是否有关联的用户，如果有则从已登录用户集合中移除
    if (!currentConnectedUser.empty())
    {
        g_loggedInUsersMutex.lock();
        auto it = g_loggedInUsers.find(currentConnectedUser);
        if (it != g_loggedInUsers.end())
        {
            g_loggedInUsers.erase(it);
            std::cout << "用户 " << currentConnectedUser << " 已因连接断开而被自动登出" << std::endl;
        }
        g_loggedInUsersMutex.unlock();
    }

    // 客户端断开连接，关闭socket
    closesocket(clientSocket);
    std::cout << "客户端连接已关闭" << std::endl;
}

int main()
{
    SetConsoleOutputCP(65001); // 设置控制台输出编码为 UTF-8
    SetConsoleCP(65001);       // 设置控制台输入编码为 UTF-8

    // 初始化 Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup 失败: " << result << std::endl;
        return 1;
    }

    // 创建服务器socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
    {
        std::cerr << "创建socket失败: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // 绑定到本地地址和端口
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888); // 使用8888端口

    if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "绑定失败: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // 开始监听
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "监听失败: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "服务器启动，等待连接..." << std::endl;

    // 加载用户数据
    vector<User *> users = User::loadUsersFromFile(USER_FILE);
    std::cout << "已加载 " << users.size() << " 个用户数据。" << std::endl;

    // 创建 Store 对象
    Store store(STORE_FILE);
    store.loadAllProducts();
    std::cout << "已加载商品数据。" << std::endl;

    // 创建订单管理器
    OrderManager orderManager(ORDER_DIR);
    orderManager.startProcessingThread(store, users);
    std::cout << "订单处理线程已启动。" << std::endl;

    std::vector<std::thread> clientThreads;

    // 主循环，接受客户端连接
    while (g_serverRunning)
    {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "接受连接失败: " << WSAGetLastError() << std::endl;
            continue;
        }

        // 为每个客户端创建新线程
        clientThreads.emplace_back(handleClient, clientSocket, std::ref(users), std::ref(store), std::ref(orderManager));
    }

    // 等待所有客户端线程结束
    for (auto &thread : clientThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    // 停止订单处理线程
    orderManager.stopProcessingThread();

    // 保存用户数据
    User::saveUsersToFile(users, USER_FILE);
    std::cout << "用户数据已保存。" << std::endl;

    // 释放资源
    for (auto *user : users)
    {
        delete user;
    }

    // 关闭socket
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}
