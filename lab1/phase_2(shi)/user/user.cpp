#include "user.h"
#include "../store/store.h" // 需要 Product 和 Store 的完整定义
#include "../order/order.h"
#include "../ordermanager/ordermanager.h"
#include <iostream>
#include <limits>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem> // 用于文件操作
#include <algorithm>  // for std::remove_if
#include <iomanip>    // for std::fixed, std::setprecision
#include <thread>

using namespace std;

// 声明在 start.cpp 中定义的全局常量
extern const string CART_FILE; // 用于 Customer 构造函数
extern const string USER_FILE; // 用于保存用户信息

// User 构造函数实现
User::User(std::string u_name, std::string pwd, double bal, bool is_customer, bool is_seller, bool is_admin)
    : username(u_name), password(pwd), balance(bal)
{
    // userType 在派生类中设置
}

User::User() : username(""), password(""), balance(0.0), userType("unknown") {}

// --- Customer 类购物车相关方法实现 ---

std::string Customer::getCartFilePath() const
{
    if (this->username.empty())
    { // 防止用户名为空时生成无效路径
        // cerr << "警告: Customer 用户名为空，无法生成购物车路径。" << endl;
        return ""; // 或者抛出异常，或者返回一个默认的无效路径
    }
    return this->cartDirectoryPath + "/" + this->username + "_cart.txt";
}

void Customer::loadCartFromFile()
{
    shoppingCartItems.clear();
    std::string cartPath = getCartFilePath();
    if (cartPath.empty())
        return; // 用户名为空，无法加载

    std::ifstream file(cartPath);
    if (!file.is_open())
    {
        return; // 文件不存在或无法打开是正常情况（购物车为空）
    }

    std::string line;
    while (getline(file, line))
    {
        std::stringstream ss(line);
        std::string productId, productName, sellerUsernameStr;
        int quantity;
        double priceAtAddition;
        if (getline(ss, productId, ',') &&
            getline(ss, productName, ',') &&
            (ss >> quantity) &&
            (ss.ignore(1) && ss >> priceAtAddition) &&
            (ss.ignore(1) && getline(ss, sellerUsernameStr)))
        {
            shoppingCartItems.emplace_back(productId, productName, quantity, priceAtAddition, sellerUsernameStr);
        }
        else
        {
            std::cerr << "警告: 购物车文件 " << cartPath << " 中存在格式错误的行: " << line << std::endl;
        }
    }
    file.close();
}

bool Customer::saveCartToFile() const
{
    std::string cartPath = getCartFilePath();
    if (cartPath.empty())
    {
        std::cerr << "错误: Customer 用户名为空，无法保存购物车。" << std::endl;
        return false;
    }

    std::ofstream file(cartPath); // 会覆盖旧文件
    if (!file.is_open())
    {
        std::cerr << "错误: 无法打开购物车文件进行写入: " << cartPath << std::endl;
        return false;
    }

    for (const auto &item : shoppingCartItems)
    {
        file << item.productId << ","
             << item.productName << ","
             << item.quantity << ","
             << item.priceAtAddition << ","
             << item.sellerUsername << std::endl;
    }
    file.close();
    return true;
}

// Customer 构造函数实现
Customer::Customer(std::string uname, std::string pwd, const std::string &cartDir, double bal)
    : User(uname, pwd, bal, true, false, false), cartDirectoryPath(cartDir)
{
    this->userType = "customer";
    if (!this->username.empty())
    { // 只有当用户名有效时才加载购物车
        loadCartFromFile();
    }
}

// 默认构造函数
Customer::Customer(const std::string &cartDir)
    : User(), cartDirectoryPath(cartDir)
{
    this->userType = "customer";
    // 注意: 此时 username 为空，loadCartFromFile 不会执行。
    // 如果后续通过 setUsername 设置了用户名，需要一种机制来加载对应的购物车。
    // 或者，强制 Customer 总是在有用户名的情况下创建。
}

bool Customer::addToCart(const Product &product, int quantity)
{
    if (this->username.empty())
    {
        cout << "错误：无法为匿名用户添加购物车。" << endl;
        return false;
    }
    if (quantity <= 0)
    {
        cout << "添加数量必须大于0。" << endl;
        return false;
    }

    bool found = false;
    for (auto &item : shoppingCartItems)
    {
        if (item.productId == product.getName())
        { // 假设商品名称是唯一ID
            item.quantity += quantity;
            // 可选：更新 priceAtAddition 为最新价格或保持不变
            found = true;
            break;
        }
    }

    if (!found)
    {
        shoppingCartItems.emplace_back(product.getName(), product.getName(), quantity, product.getPrice(), product.getSellerUsername());
    }
    cout << "\"" << product.getName() << "\" 已成功加入购物车。" << endl;
    return saveCartToFile();
}

bool Customer::removeCartItem(const std::string &productName)
{
    if (this->username.empty())
        return false;
    auto initial_size = shoppingCartItems.size();
    shoppingCartItems.erase(std::remove_if(shoppingCartItems.begin(), shoppingCartItems.end(),
                                           [&](const CartItem &item)
                                           { return item.productName == productName; }),
                            shoppingCartItems.end());

    if (shoppingCartItems.size() < initial_size)
    {
        cout << "商品 \"" << productName << "\" 已从购物车移除。" << endl;
        return saveCartToFile();
    }
    else
    {
        cout << "未在购物车中找到商品: " << productName << endl;
        return false;
    }
}

bool Customer::updateCartItemQuantity(const std::string &productName, int newQuantity, Store &store)
{
    if (this->username.empty())
        return false;
    if (newQuantity < 0)
    {
        cout << "数量不能为负。" << endl;
        return false;
    }
    bool updated = false;
    for (auto &item : shoppingCartItems)
    {
        if (item.productName == productName)
        {
            Product *p_info = store.findProductByName(productName); // 检查最新库存
            if (p_info && newQuantity > p_info->getQuantity())
            {
                cout << "库存不足！无法将购物车中 \"" << productName << "\" 的数量修改为 " << newQuantity
                     << "。当前库存: " << p_info->getQuantity() << endl;
                return false;
            }
            item.quantity = newQuantity;
            updated = true;
            break;
        }
    }

    if (!updated)
    {
        cout << "未在购物车中找到商品: " << productName << endl;
        return false;
    }

    // 移除数量为0的商品
    shoppingCartItems.erase(std::remove_if(shoppingCartItems.begin(), shoppingCartItems.end(),
                                           [](const CartItem &item)
                                           { return item.quantity <= 0; }),
                            shoppingCartItems.end());

    cout << "购物车商品 \"" << productName << "\" 数量已更新。" << endl;
    return saveCartToFile();
}

void Customer::clearCartAndFile()
{
    if (this->username.empty())
        return;
    shoppingCartItems.clear();
    std::string cartPath = getCartFilePath();
    if (!cartPath.empty() && std::filesystem::exists(cartPath))
    {
        try
        {
            std::filesystem::remove(cartPath);
        }
        catch (const std::filesystem::filesystem_error &err)
        {
            std::cerr << "错误: 删除购物车文件失败: " << err.what() << std::endl;
        }
    }
}

bool Customer::isCartEmpty() const
{
    return shoppingCartItems.empty();
}

void Customer::viewCart(Store &store, std::vector<User *> &allUsers)
{
    if (this->username.empty())
    {
        cout << "错误：匿名用户无法查看购物车。" << endl;
        return;
    }
    // 确保购物车是最新的（如果其他地方可能修改文件但未更新内存）
    // loadCartFromFile(); // 通常在每次操作后保存，所以这里可能不需要，除非有外部修改购物车的可能

    if (shoppingCartItems.empty())
    {
        cout << "\n您的购物车是空的。" << endl;
        return;
    }

    double totalCartPriceBasedOnCurrent = 0;
    cout << "\n--- 我的购物车 ---" << endl;
    cout << std::fixed << std::setprecision(2); // 设置输出格式
    for (size_t i = 0; i < shoppingCartItems.size(); ++i)
    {
        const auto &item = shoppingCartItems[i];
        Product *p_info = store.findProductByName(item.productId);
        double currentItemPrice = p_info ? p_info->getPrice() : item.priceAtAddition;

        cout << i + 1 << ". 商品: " << item.productName
             << " | 数量: " << item.quantity
             << " | 当前单价: ¥" << currentItemPrice
             << " | 小计: ¥" << (item.quantity * currentItemPrice) << endl;
        if (p_info)
        {
            if (p_info->getQuantity() < item.quantity)
            {
                cout << "   注意: " << item.productName << " 当前库存(" << p_info->getQuantity() << ")不足购物车数量(" << item.quantity << ")!" << endl;
            }
            if (p_info->getPrice() != item.priceAtAddition)
            {
                cout << "   提示: " << item.productName << " 加入时价格为 ¥" << item.priceAtAddition << ", 当前价格已变为 ¥" << p_info->getPrice() << endl;
            }
        }
        else
        {
            cout << "   警告: 商品 " << item.productName << " 可能已从商店下架!" << endl;
        }
        totalCartPriceBasedOnCurrent += item.quantity * currentItemPrice;
    }
    cout << "--------------------" << endl;
    cout << "购物车总计 (按当前价格): ¥" << totalCartPriceBasedOnCurrent << endl;
    cout << "--------------------" << endl;

    int cartChoice = -1;
    do
    {
        cout << "\n购物车操作:" << endl;
        cout << "1. 修改商品数量" << endl;
        cout << "2. 移除商品" << endl;
        cout << "3. 生成订单并结算" << endl;
        cout << "0. 返回商城" << endl;
        cout << "请选择: ";

        if (!(cin >> cartChoice))
        {
            cout << "无效输入。" << endl;
            User::clearInputBuffer();
            cartChoice = -1;
            continue;
        }
        User::clearInputBuffer();

        switch (cartChoice)
        {
        case 1:
        {
            string nameToUpdate;
            int newQty;
            cout << "输入要修改数量的商品名称: ";
            getline(cin >> ws, nameToUpdate);
            cout << "输入新的数量 (输入0将移除该商品): ";
            if (!(cin >> newQty))
            {
                cout << "无效数量输入。" << endl;
                User::clearInputBuffer();
                break;
            }
            User::clearInputBuffer();
            if (updateCartItemQuantity(nameToUpdate, newQty, store))
            {
                // 成功后重新加载并显示购物车 (递归调用或重新加载数据)
                // 为了避免无限递归，这里直接返回，让外层循环重新调用 viewCart
                cout << "购物车已更新。" << endl;
            }
            return viewCart(store, allUsers); // 重新显示更新后的购物车
        }
        case 2:
        {
            string nameToRemove;
            cout << "输入要移除的商品名称: ";
            getline(cin >> ws, nameToRemove);
            if (removeCartItem(nameToRemove))
            {
                cout << "购物车已更新。" << endl;
            }
            return viewCart(store, allUsers); // 重新显示更新后的购物车
        }
        case 3: // Generate Order and Checkout
        {
            if (shoppingCartItems.empty())
            {
                cout << "购物车是空的，无法生成订单。" << endl;
                break;
            }

            Order currentOrder(this->getUsername()); // Create a new order object
            currentOrder.setStatus("PENDING_VALIDATION");

            // 第一阶段：验证商品并构建订单（预检查）
            bool preCheckOk = true;
            for (const auto &cart_item : shoppingCartItems)
            {
                Product *product = store.findProductByName(cart_item.productId);
                if (!product)
                {
                    cout << "错误: 购物车商品 \"" << cart_item.productName << "\" 已不存在。" << endl;
                    currentOrder.setStatus("CANCELLED_PRODUCT_NOT_FOUND");
                    preCheckOk = false;
                    break;
                }
                if (product->getQuantity() < cart_item.quantity)
                {
                    cout << "错误: 购物车商品 \"" << cart_item.productName << "\" 库存不足 (需要 "
                         << cart_item.quantity << ", 现有 " << product->getQuantity() << ")。" << endl;
                    currentOrder.setStatus("CANCELLED_INSUFFICIENT_STOCK");
                    preCheckOk = false;
                    break;
                }
                currentOrder.addItemFromProduct(*product, cart_item.quantity);
            }

            if (!preCheckOk)
            {
                currentOrder.displaySummary(); // Display order with cancelled status
                cout << "订单无法继续，请调整购物车后重试。" << endl;
                break; // Back to cart menu
            }

            // 第二阶段：验证客户余额（只是初步检查，队列处理时还会再次检查）
            if (this->checkBalance() < currentOrder.getTotalAmount())
            {
                cout << "错误: 您的余额不足以支付此订单！" << endl;
                currentOrder.setStatus("CANCELLED_INSUFFICIENT_FUNDS");
                // currentOrder.displaySummary();
                cout << "您的余额:   ¥" << this->checkBalance() << endl;
                break; // Back to cart menu
            }

            // 第三阶段：显示订单并请求确认
            currentOrder.setStatus("PENDING_CONFIRMATION");
            currentOrder.displaySummary();
            cout << "您的当前余额: ¥" << std::fixed << std::setprecision(2) << this->checkBalance() << endl;
            cout << "支付后余额将为: ¥" << (this->checkBalance() - currentOrder.getTotalAmount()) << endl;
            cout << "--------------------" << endl;

            char confirmChoice;
            cout << "确认支付并完成订单吗? (y/n): ";
            cin >> confirmChoice;
            User::clearInputBuffer();

            if (tolower(confirmChoice) != 'y')
            {
                currentOrder.setStatus("CANCELLED_BY_USER");
                currentOrder.displaySummary();
                cout << "订单已取消。" << endl;
                break; // Back to cart menu
            }

            // Stage 4: Process payment and update inventory (Actual "lock" and transaction)
            cout << "\n正在处理订单，请稍候..." << endl;
            currentOrder.setStatus("PROCESSING");

            // 访问OrderManager，提交订单
            extern OrderManager g_orderManager;                                      // 全局变量声明
            Order &submittedOrder = g_orderManager.submitOrderRequest(currentOrder); // 获取下一个订单
            cout << "订单已成功提交到处理队列，订单ID: " << currentOrder.getOrderId() << endl;

            cout << "正在等待订单处理，请稍候..." << endl;

            // 等待订单处理完成
            while (!submittedOrder.getProcessed())
            {
                // 短暂休眠，避免过度消耗CPU
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            cout << "订单已处理完成! 状态: " << submittedOrder.getStatus() << endl;

            if (submittedOrder.getStatus() == "COMPLETED" ||
                submittedOrder.getStatus() == "COMPLETED_WITH_PAYMENT_ISSUES")
            {
                clearCartAndFile(); // 成功提交后清空购物车
            }

            cout << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
            return; // 退出viewCart，返回Customer_page商店菜单

            // if (!this->withdraw(currentOrder.getTotalAmount()))
            // {
            //     currentOrder.setStatus("FAILED_PAYMENT_WITHDRAWAL");
            //     currentOrder.displaySummary();
            //     cout << "错误: 支付失败。请检查余额或联系客服。" << endl;
            //     // Potentially log this critical failure
            //     break;
            // }

            // for (const auto &order_item : currentOrder.getItems())
            // {
            //     Product *product = store.findProductByName(order_item.productId); // Re-fetch, though should be same
            //     if (product)
            //     { // Should always be true due to pre-checks
            //         product->setQuantity(product->getQuantity() - order_item.quantity);

            //         User *seller = User::findUser(allUsers, order_item.sellerUsername);
            //         if (seller)
            //         {
            //             if (!seller->deposit(order_item.priceAtPurchase * order_item.quantity))
            //             {
            //                 cerr << "警告: 向商家 " << seller->getUsername() << " 转账失败 (商品: "
            //                      << order_item.productName << "). 订单号: " << currentOrder.getOrderId() << endl;
            //                 // Log this for reconciliation
            //             }
            //         }
            //         else if (!order_item.sellerUsername.empty())
            //         {
            //             cerr << "警告: 未找到商品 \"" << order_item.productName << "\" 的商家 \""
            //                  << order_item.sellerUsername << "\". 订单号: " << currentOrder.getOrderId() << endl;
            //         }
            //         else
            //         {
            //             cerr << "警告: 商品 \"" << order_item.productName << "\" 没有商家信息. 订单号: " << currentOrder.getOrderId() << endl;
            //         }
            //     }
            // }

            // User::saveUsersToFile(allUsers, USER_FILE);
            // store.saveAllProducts(); // Or save only affected sellers

            // currentOrder.setStatus("COMPLETED");
            // cout << "\n订单处理完成！" << endl;
            // currentOrder.displaySummary(); // Display final completed order
            // // Optional: currentOrder.saveToFile(ORDER_DIRECTORY_BASE + "/" + this->getUsername());

            // clearCartAndFile(); // Clear cart after successful order

            // cout << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
            // return; // Exit viewCart, back to Customer_page store menu
        }
        case 0:
            // 返回商城，什么也不做，循环会结束
            break;
        default:
            cout << "无效选项。" << endl;
        }
    } while (cartChoice != 0);
    cout << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield); // 重置输出格式
}

// Seller 构造函数实现
Seller::Seller(std::string uname, std::string pwd, double bal)
    : User(uname, pwd, bal, false, true, false)
{
    this->userType = "seller";
}
Seller::Seller() : User() { this->userType = "seller"; }

// Admin 构造函数实现
Admin::Admin(std::string uname, std::string pwd, double bal)
    : User(uname, pwd, bal, false, false, true)
{
    this->userType = "admin";
}
Admin::Admin() : User() { this->userType = "admin"; }

// --- 静态方法：用户注册实现 ---
User *User::registerUser(std::vector<User *> &users)
{
    string newUsername, newPassword;
    double initialBalance = 0.0;
    int userTypeChoice;

    cout << "请输入新用户名: ";
    getline(cin >> ws, newUsername);
    if (isUsernameExists(users, newUsername))
    {
        cout << "用户名已存在！" << endl;
        return nullptr;
    }
    cout << "请输入密码: ";
    getline(cin >> ws, newPassword);

    cout << "选择用户类型 (1: 消费者, 2: 商家): ";
    if (!(cin >> userTypeChoice) || (userTypeChoice != 1 && userTypeChoice != 2))
    {
        cout << "无效的用户类型选择。" << endl;
        clearInputBuffer();
        return nullptr;
    }
    clearInputBuffer();

    User *newUser = nullptr;
    if (userTypeChoice == 1)
    {                                                                                // Customer
        newUser = new Customer(newUsername, newPassword, CART_FILE, initialBalance); // 使用 CART_FILE
    }
    else
    { // Seller
        newUser = new Seller(newUsername, newPassword, initialBalance);
    }

    if (newUser)
    {
        users.push_back(newUser);
        cout << (userTypeChoice == 1 ? "消费者" : "商家") << "用户 \"" << newUsername << "\" 注册成功！" << endl;
    }
    return newUser;
}

// 从文件加载用户信息
std::vector<User *> User::loadUsersFromFile(const std::string &filename)
{
    std::vector<User *> loadedUsers;
    std::ifstream file(filename);
    if (!file.is_open())
    {
        return loadedUsers;
    }
    std::string line;
    while (getline(file, line))
    {
        std::stringstream ss(line);
        std::string uname, pwd, type;
        double bal;
        if (
            getline(ss, uname, ',') &&
            getline(ss, pwd, ',') &&
            getline(ss, type, ',') &&

            (ss >> bal))
        {
            if (type == "customer")
            {
                loadedUsers.push_back(new Customer(uname, pwd, CART_FILE, bal)); // 使用 CART_FILE
            }
            else if (type == "seller")
            {
                loadedUsers.push_back(new Seller(uname, pwd, bal));
            }
            else if (type == "admin")
            {
                loadedUsers.push_back(new Admin(uname, pwd, bal));
            }
        }
    }
    file.close();
    return loadedUsers;
}

// --- User 类其他成员函数实现 ---
// getUsername, getPassword, displayInfo, changePassword, deposit, withdraw, checkBalance,
// saveUsersToFile, findUser, isUsernameExists, login
// 这些方法的实现应该保持不变，除非它们需要与新的购物车逻辑交互（但看起来不需要）。

std::string User::getUsername() const { return username; }
std::string User::getPassword() const { return password; } // 密码不应这样直接返回，仅为示例
void User::setUsername(const std::string &newUsername) { username = newUsername; }
void User::setPassword(const std::string &newPassword) { password = newPassword; }
void User::setBalance(double newBalance)
{
    if (newBalance >= 0)
        balance = newBalance;
}

void User::displayInfo(int index) const
{
    if (index != -1)
    {
        cout << "用户 " << index << ": ";
    }
    cout << "用户名: " << username << ", 类型: " << userType << ", 余额: " << balance << endl;
}

bool User::changePassword(const std::string &oldPassword, const std::string &newPassword)
{
    if (this->password == oldPassword)
    {
        this->password = newPassword;
        return true;
    }
    cout << "原密码错误！" << endl;
    return false;
}

bool User::deposit(double amount)
{
    if (amount <= 0)
    {
        cout << "充值金额必须为正！" << endl;
        return false;
    }
    balance += amount;
    return true;
}

bool User::withdraw(double amount)
{
    if (amount <= 0)
    {
        cout << "消费金额必须为正！" << endl;
        return false;
    }
    if (balance < amount)
    {
        cout << "余额不足！" << endl;
        return false;
    }
    balance -= amount;
    return true;
}

double User::checkBalance() const
{
    return balance;
}

bool User::saveUsersToFile(const std::vector<User *> &users, const std::string &filename)
{
    ofstream file(filename);
    if (!file.is_open())
    {
        cerr << "错误: 无法打开用户文件进行写入: " << filename << endl;
        return false;
    }
    for (const auto *user : users)
    {
        if (user)
        {
            file << user->getUsername() << ","
                 << user->getPassword() << ","
                 << user->getUserType() << ","
                 << user->checkBalance() << endl;
        }
    }
    file.close();
    return true;
}

User *User::findUser(const std::vector<User *> &users, const std::string &findUsername)
{
    for (User *user : users)
    {
        if (user->getUsername() == findUsername)
        {
            return user;
        }
    }
    return nullptr;
}

bool User::isUsernameExists(const std::vector<User *> &users, const std::string &checkUsername)
{
    return findUser(users, checkUsername) != nullptr;
}

User *User::userLogin(const std::vector<User *> &users)
{
    string loginUsername, loginPassword;
    cout << "请输入用户名: ";
    getline(cin >> ws, loginUsername);
    cout << "请输入密码: ";
    getline(cin >> ws, loginPassword);

    User *foundUser = findUser(users, loginUsername);
    if (foundUser && foundUser->getPassword() == loginPassword)
    {
        cout << "登录成功！欢迎, " << loginUsername << "!" << endl;
        // 如果是 Customer，并且之前用户名为空（例如通过默认构造后设置用户名），则加载其购物车
        if (Customer *cust = dynamic_cast<Customer *>(foundUser))
        {
            if (cust->shoppingCartItems.empty() && !cust->getUsername().empty())
            {                             // 假设 shoppingCartItems 为空表示未加载
                                          // cust->cartDirectoryPath 应该在构造时已设置
                cust->loadCartFromFile(); // 尝试加载购物车
            }
        }
        return foundUser;
    }
    cout << "用户名或密码错误！" << endl;
    return nullptr;
}

User *User::login(const std::vector<User *> &users, const std::string &username_param, const std::string &password_param)
{
    User *foundUser = findUser(users, username_param);
    if (foundUser && foundUser->getPassword() == password_param)
    {
        return foundUser;
    }
    return nullptr;
}

void User::displayAllUsers(const std::vector<User *> &users)
{
    if (users.empty())
    {
        cout << "当前没有注册用户。" << endl;
        return;
    }
    cout << "\n--- 所有用户信息 ---" << endl;
    for (size_t i = 0; i < users.size(); ++i)
    {
        users[i]->displayInfo(i + 1);
    }
    cout << "--------------------" << endl;
}
// ...existing code...