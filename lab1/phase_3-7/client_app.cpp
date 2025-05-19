#include "client_app.h"
#include "client_connection.h"
#include "client_ui.h"
#include <map>

/**
 * @brief 构造函数
 */
ClientApp::ClientApp() : m_currentUsername(""),
                         m_currentUserType(""),
                         m_isLoggedIn(false)
{
    m_connection = new ClientConnection(this);
    m_ui = new ClientUI(this);
}

/**
 * @brief 析构函数
 */
ClientApp::~ClientApp()
{
    if (m_connection)
    {
        delete m_connection;
        m_connection = nullptr;
    }

    if (m_ui)
    {
        delete m_ui;
        m_ui = nullptr;
    }
}

/**
 * @brief 运行应用程序
 * @return 退出码
 */
int ClientApp::run()
{
    // 连接到服务器
    if (!m_connection->connectToServer())
    {
        return 1;
    }

    int choice;

    do
    {
        choice = m_ui->showMainMenu();

        switch (choice)
        {
        case 1:
            handleLogin();
            break;
        case 2:
            handleRegister();
            break;
        case 0:
            std::cout << "感谢使用，再见！" << std::endl;
            break;
        default:
            std::cout << "无效选择，请重试。" << std::endl;
        }
    } while (choice != 0 && !m_isLoggedIn);

    // 用户已登录，显示对应菜单
    while (m_isLoggedIn)
    {
        if (m_currentUserType == "customer")
        {
            choice = m_ui->showCustomerMenu(m_currentUsername);

            switch (choice)
            {
            case 1:
                handleViewUserInfo();
                break;
            case 2:
                handleCheckBalance();
                break;
            case 3:
                handleDeposit();
                break;
            case 4:
                handleEnterStore();
                break;
            case 5:
                handleViewCart();
                break;
            case 6:
                handleLogout();
                break;
            case 0:
                std::cout << "感谢使用，再见！" << std::endl;
                m_isLoggedIn = false;
                break;
            default:
                std::cout << "无效选择，请重试。" << std::endl;
            }
        }
        else if (m_currentUserType == "seller")
        {
            choice = m_ui->showSellerMenu(m_currentUsername);

            switch (choice)
            {
            case 1:
                handleSellerInfo();
                break;
            case 2:
                handleChangePassword();
                break;
            case 3:
                handleEnterStore();
                break;
            case 4:
                handleCheckIncome();
                break;
            case 5:
                handleLogout();
                break;
            case 0:
                std::cout << "感谢使用，再见！" << std::endl;
                m_isLoggedIn = false;
                break;
            default:
                std::cout << "无效选择，请重试。" << std::endl;
            }
        }
    }

    // 断开连接
    m_connection->disconnect();

    return 0;
}

/**
 * @brief 获取当前用户名
 * @return 当前用户名
 */
std::string ClientApp::getCurrentUsername() const
{
    return m_currentUsername;
}

/**
 * @brief 获取当前用户类型
 * @return 当前用户类型
 */
std::string ClientApp::getCurrentUserType() const
{
    return m_currentUserType;
}

/**
 * @brief 检查是否已登录
 * @return 是否已登录
 */
bool ClientApp::isLoggedIn() const
{
    return m_isLoggedIn;
}

/**
 * @brief 设置当前用户名
 * @param username 用户名
 */
void ClientApp::setCurrentUsername(const std::string &username)
{
    m_currentUsername = username;
}

/**
 * @brief 设置当前用户类型
 * @param userType 用户类型
 */
void ClientApp::setCurrentUserType(const std::string &userType)
{
    m_currentUserType = userType;
}

/**
 * @brief 设置登录状态
 * @param isLoggedIn 是否已登录
 */
void ClientApp::setLoggedIn(bool isLoggedIn)
{
    m_isLoggedIn = isLoggedIn;
}

/**
 * @brief 获取连接对象
 * @return 连接对象
 */
ClientConnection *ClientApp::getConnection() const
{
    return m_connection;
}

// 实现各种功能方法
void ClientApp::handleLogin()
{
    std::string username, password;

    std::cout << "\n===== 用户登录 =====\n";
    std::cout << "用户名: ";
    std::getline(std::cin, username);
    std::cout << "密码: ";
    std::getline(std::cin, password);

    std::string request = "LOGIN|" + username + "|" + password;
    std::string response;

    if (m_connection->sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, returnedUsername, userType;

        std::getline(iss, status, '|');
        std::getline(iss, returnedUsername, '|');
        std::getline(iss, userType, '|');

        if (status == "LOGIN_SUCCESS")
        {
            m_currentUsername = returnedUsername;
            m_currentUserType = userType;
            m_isLoggedIn = true;
            std::cout << "登录成功。欢迎, " << returnedUsername << " (" << userType << ")" << std::endl;
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "登录失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }
}

void ClientApp::handleRegister()
{
    std::string username, password, confirmPassword, typeChoice;
    int choice;

    std::cout << "\n===== 用户注册 =====\n";
    std::cout << "用户名: ";
    std::getline(std::cin, username);
    std::cout << "密码: ";
    std::getline(std::cin, password);
    std::cout << "确认密码: ";
    std::getline(std::cin, confirmPassword);

    if (password != confirmPassword)
    {
        std::cout << "两次输入的密码不一致。" << std::endl;
        return;
    }

    std::cout << "账户类型 (1: 消费者, 2: 商家): ";
    std::cin >> choice;
    m_ui->clearInputBuffer();

    if (choice == 1)
    {
        typeChoice = "customer";
    }
    else if (choice == 2)
    {
        typeChoice = "seller";
    }
    else
    {
        std::cout << "无效账户类型。" << std::endl;
        return;
    }

    std::string request = "REGISTER|" + username + "|" + password + "|" + typeChoice;
    std::string response;

    if (m_connection->sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, returnedUsername;

        std::getline(iss, status, '|');
        std::getline(iss, returnedUsername, '|');

        if (status == "REGISTER_SUCCESS")
        {
            std::cout << "注册成功。请使用新账户登录。" << std::endl;
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "注册失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }
}

// 从client_app_impl.h迁移的方法实现
void ClientApp::handleBrowseProducts()
{
    std::string request = "GET_PRODUCTS";
    std::string response;

    if (m_connection->sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, countStr;

        std::getline(iss, status, '|');
        std::getline(iss, countStr, '|');

        if (status == "PRODUCTS")
        {
            int count = std::stoi(countStr);
            std::cout << "\n===== 商品列表 =====\n";
            std::cout << "找到 " << count << " 个商品\n";

            if (count > 0)
            {
                std::cout << std::left << std::setw(20) << "商品名称" << std::setw(10) << "类型" << std::setw(10) << "价格" << std::setw(10) << "库存" << std::setw(15) << "商家" << std::endl;
                std::cout << std::string(65, '-') << std::endl;

                for (int i = 0; i < count; i++)
                {
                    std::string productInfo;
                    std::getline(iss, productInfo, '|');

                    std::istringstream productIss(productInfo);
                    std::string name, type, price, quantity, seller;

                    std::getline(productIss, name, ',');
                    std::getline(productIss, type, ',');
                    std::getline(productIss, price, ',');
                    std::getline(productIss, quantity, ',');
                    std::getline(productIss, seller, ',');

                    std::cout << std::left << std::setw(20) << name << std::setw(10) << type << std::setw(10) << price << std::setw(10) << quantity << std::setw(15) << seller << std::endl;
                }
            }
        }
        else
        {
            std::cout << "获取商品列表失败。" << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    // 简单的暂停
    std::cout << "\n按回车键继续...";
    std::cin.get();
}

// 仅实现简单版本的其他方法
void ClientApp::handleProductDetail()
{
    std::string productName, sellerName;

    std::cout << "\n===== 商品详情 =====\n";
    std::cout << "输入商品名称: ";
    std::getline(std::cin, productName);
    std::cout << "输入商家名称: ";
    std::getline(std::cin, sellerName);

    std::string request = "GET_PRODUCT_DETAIL|" + productName + "|" + sellerName;
    std::string response;

    if (m_connection->sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status;

        std::getline(iss, status, '|');

        if (status == "PRODUCT_DETAIL")
        {
            std::string name, type, description, priceStr, quantityStr, seller;

            std::getline(iss, name, '|');
            std::getline(iss, type, '|');
            std::getline(iss, description, '|');
            std::getline(iss, priceStr, '|');
            std::getline(iss, quantityStr, '|');
            std::getline(iss, seller, '|');

            std::cout << "商品名称: " << name << "\n";
            std::cout << "类型: " << type << "\n";
            std::cout << "描述: " << description << "\n";
            std::cout << "价格: " << priceStr << " 元\n";
            std::cout << "库存: " << quantityStr << "\n";
            std::cout << "商家: " << seller << "\n";

            if (type == "Book")
            {
                std::string author, isbn;
                std::getline(iss, author, '|');
                std::getline(iss, isbn, '|');
                std::cout << "作者: " << author << "\n";
                std::cout << "ISBN: " << isbn << "\n";
            }
            else if (type == "Clothing")
            {
                std::string size, color;
                std::getline(iss, size, '|');
                std::getline(iss, color, '|');
                std::cout << "尺寸: " << size << "\n";
                std::cout << "颜色: " << color << "\n";
            }
            else if (type == "Food")
            {
                std::string expDate;
                std::getline(iss, expDate, '|');
                std::cout << "保质期: " << expDate << "\n";
            }
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "获取商品详情失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void ClientApp::handleViewCart()
{
    if (!m_isLoggedIn || m_currentUserType != "customer")
    {
        std::cout << "请先以消费者身份登录。" << std::endl;
        return;
    }

    std::string request = "GET_CART|" + m_currentUsername;
    std::string response;

    if (m_connection->sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, countStr;

        std::getline(iss, status, '|');

        if (status == "CART")
        {
            std::getline(iss, countStr, '|');
            int count = std::stoi(countStr);

            if (count == 0)
            {
                std::cout << "\n您的购物车是空的。" << std::endl;
                std::cout << "按回车键继续...";
                std::cin.get();
                return;
            }

            // 解析购物车项目
            std::vector<std::pair<std::string, std::string>> cartItems; // 商品名称，商家名称
            std::vector<int> quantities;                                // 数量
            std::vector<double> prices;                                 // 单价
            double totalAmount = 0.0;

            std::cout << "\n--- 我的购物车 ---" << std::endl;
            std::cout << std::fixed << std::setprecision(2); // 设置输出格式

            for (int i = 0; i < count; i++)
            {
                std::string item;
                std::getline(iss, item, '|');

                std::istringstream itemStream(item);
                std::string name, quantityStr, priceStr, seller;

                std::getline(itemStream, name, ',');
                std::getline(itemStream, quantityStr, ',');
                std::getline(itemStream, priceStr, ',');
                std::getline(itemStream, seller, ',');

                int quantity = std::stoi(quantityStr);
                double price = std::stod(priceStr);
                double itemTotal = quantity * price;
                totalAmount += itemTotal;

                cartItems.push_back(std::make_pair(name, seller));
                quantities.push_back(quantity);
                prices.push_back(price);

                std::cout << i + 1 << ". 商品: " << name
                          << " | 数量: " << quantity
                          << " | 当前单价: ¥" << price
                          << " | 小计: ¥" << itemTotal << std::endl;

                // 获取商品详情，检查库存和价格变化
                std::string detailRequest = "GET_PRODUCT_DETAIL|" + name + "|" + seller;
                std::string detailResponse;

                if (m_connection->sendRequest(detailRequest, detailResponse))
                {
                    std::istringstream detailIss(detailResponse);
                    std::string detailStatus;
                    std::getline(detailIss, detailStatus, '|');

                    if (detailStatus == "PRODUCT_DETAIL")
                    {
                        std::string productName, productType, productDesc, currentPriceStr, currentQuantityStr, productSeller;
                        std::getline(detailIss, productName, '|');
                        std::getline(detailIss, productType, '|');
                        std::getline(detailIss, productDesc, '|');
                        std::getline(detailIss, currentPriceStr, '|');
                        std::getline(detailIss, currentQuantityStr, '|');

                        double currentPrice = std::stod(currentPriceStr);
                        int currentQuantity = std::stoi(currentQuantityStr);

                        if (currentQuantity < quantity)
                        {
                            std::cout << "   注意: " << name << " 当前库存(" << currentQuantity
                                      << ")不足购物车数量(" << quantity << ")!" << std::endl;
                        }

                        if (currentPrice != price)
                        {
                            std::cout << "   提示: " << name << " 加入时价格为 ¥" << price
                                      << ", 当前价格已变为 ¥" << currentPrice << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "   警告: 商品 " << name << " 可能已从商店下架!" << std::endl;
                    }
                }
            }

            std::cout << "--------------------" << std::endl;
            std::cout << "购物车总计: ¥" << totalAmount << std::endl;
            std::cout << "--------------------" << std::endl;

            // 购物车操作菜单
            int cartChoice = -1;
            do
            {
                std::cout << "\n购物车操作:" << std::endl;
                std::cout << "1. 修改商品数量" << std::endl;
                std::cout << "2. 移除商品" << std::endl;
                std::cout << "3. 生成订单并结算" << std::endl;
                std::cout << "0. 返回商城" << std::endl;
                std::cout << "请选择: ";

                if (!(std::cin >> cartChoice))
                {
                    std::cout << "无效输入。" << std::endl;
                    m_ui->clearInputBuffer();
                    cartChoice = -1;
                    continue;
                }
                m_ui->clearInputBuffer();

                switch (cartChoice)
                {
                case 1: // 修改商品数量
                {
                    std::string nameToUpdate;
                    std::string sellerName;
                    int newQty;

                    std::cout << "输入要修改数量的商品名称: ";
                    std::getline(std::cin >> std::ws, nameToUpdate);

                    // 查找商品对应的商家
                    bool found = false;
                    for (size_t i = 0; i < cartItems.size(); i++)
                    {
                        if (cartItems[i].first == nameToUpdate)
                        {
                            sellerName = cartItems[i].second;
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        std::cout << "未在购物车中找到商品: " << nameToUpdate << std::endl;
                        break;
                    }

                    std::cout << "输入新的数量 (输入0将移除该商品): ";
                    if (!(std::cin >> newQty) || newQty < 0)
                    {
                        std::cout << "无效数量输入。" << std::endl;
                        m_ui->clearInputBuffer();
                        break;
                    }
                    m_ui->clearInputBuffer();

                    // 发送更新请求
                    std::string updateRequest;
                    if (newQty == 0)
                    {
                        updateRequest = "REMOVE_FROM_CART|" + m_currentUsername + "|" + nameToUpdate + "|" + sellerName;
                    }
                    else
                    {
                        updateRequest = "UPDATE_CART_ITEM|" + m_currentUsername + "|" + nameToUpdate + "|" + sellerName + "|" + std::to_string(newQty);
                    }

                    std::string updateResponse;
                    if (m_connection->sendRequest(updateRequest, updateResponse))
                    {
                        std::istringstream updateIss(updateResponse);
                        std::string updateStatus, updateMessage;

                        std::getline(updateIss, updateStatus, '|');
                        std::getline(updateIss, updateMessage, '|');

                        if (updateStatus == "CART_UPDATED")
                        {
                            std::cout << "购物车已更新: " << updateMessage << std::endl;
                            handleViewCart(); // 刷新购物车
                            return;
                        }
                        else
                        {
                            std::cout << "更新失败: " << updateMessage << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "与服务器通信失败。" << std::endl;
                    }
                    break;
                }
                case 2: // 移除商品
                {
                    std::string nameToRemove;
                    std::string sellerName;

                    std::cout << "输入要移除的商品名称: ";
                    std::getline(std::cin >> std::ws, nameToRemove);

                    // 查找商品对应的商家
                    bool found = false;
                    for (size_t i = 0; i < cartItems.size(); i++)
                    {
                        if (cartItems[i].first == nameToRemove)
                        {
                            sellerName = cartItems[i].second;
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        std::cout << "未在购物车中找到商品: " << nameToRemove << std::endl;
                        break;
                    }

                    // 发送移除请求
                    std::string removeRequest = "REMOVE_FROM_CART|" + m_currentUsername + "|" + nameToRemove + "|" + sellerName;
                    std::string removeResponse;

                    if (m_connection->sendRequest(removeRequest, removeResponse))
                    {
                        std::istringstream removeIss(removeResponse);
                        std::string removeStatus, removeMessage;

                        std::getline(removeIss, removeStatus, '|');
                        std::getline(removeIss, removeMessage, '|');

                        if (removeStatus == "CART_UPDATED")
                        {
                            std::cout << "购物车已更新: " << removeMessage << std::endl;
                            handleViewCart(); // 刷新购物车
                            return;
                        }
                        else
                        {
                            std::cout << "移除失败: " << removeMessage << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "与服务器通信失败。" << std::endl;
                    }
                    break;
                }
                case 3: // 生成订单并结算
                    handleCheckout(cartItems, quantities, prices, totalAmount);
                    return;
                case 0: // 返回商城
                    break;
                default:
                    std::cout << "无效选项。" << std::endl;
                }

            } while (cartChoice != 0);
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "获取购物车失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void ClientApp::handleAddToCart()
{
    if (!m_isLoggedIn || m_currentUserType != "customer")
    {
        std::cout << "请先以消费者身份登录。" << std::endl;
        return;
    }

    std::string productName, sellerName;
    int quantity;

    std::cout << "\n===== 添加商品到购物车 =====\n";
    std::cout << "输入商品名称: ";
    std::getline(std::cin, productName);
    std::cout << "输入商家名称: ";
    std::getline(std::cin, sellerName);
    std::cout << "输入数量: ";
    if (!(std::cin >> quantity) || quantity <= 0)
    {
        std::cout << "无效数量。" << std::endl;
        m_ui->clearInputBuffer();
        return;
    }
    m_ui->clearInputBuffer();

    std::string request = "ADD_TO_CART|" + m_currentUsername + "|" + productName + "|" + sellerName + "|" + std::to_string(quantity);
    std::string response;

    if (m_connection->sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, message;

        std::getline(iss, status, '|');
        std::getline(iss, message, '|');

        if (status == "CART_UPDATED")
        {
            std::cout << "成功添加到购物车: " << message << std::endl;
        }
        else
        {
            std::cout << "添加到购物车失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void ClientApp::handleCheckout(const std::vector<std::pair<std::string, std::string>> &cartItems,
                               const std::vector<int> &quantities,
                               const std::vector<double> &prices,
                               double totalAmount)
{
    // 首先获取用户余额
    std::string balanceRequest = "CHECK_BALANCE|" + m_currentUsername;
    std::string balanceResponse;
    double balance = 0.0;

    if (m_connection->sendRequest(balanceRequest, balanceResponse))
    {
        std::istringstream balanceIss(balanceResponse);
        std::string balanceStatus, balanceStr;

        std::getline(balanceIss, balanceStatus, '|');
        if (balanceStatus == "BALANCE")
        {
            std::getline(balanceIss, balanceStr, '|');
            balance = std::stod(balanceStr);
        }
    }

    // 显示订单预览
    std::cout << "\n===== 订单预览 =====\n";
    for (size_t i = 0; i < cartItems.size(); i++)
    {
        std::cout << i + 1 << ". " << cartItems[i].first
                  << " x " << quantities[i]
                  << " @ ¥" << prices[i]
                  << " = ¥" << (quantities[i] * prices[i]) << std::endl;
    }
    std::cout << "--------------------" << std::endl;
    std::cout << "订单总计: ¥" << totalAmount << std::endl;
    std::cout << "您的当前余额: ¥" << std::fixed << std::setprecision(2) << balance << std::endl;
    std::cout << "支付后余额将为: ¥" << (balance - totalAmount) << std::endl;
    std::cout << "--------------------" << std::endl;

    if (balance < totalAmount)
    {
        std::cout << "错误: 您的余额不足以支付此订单！" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 确认支付
    char confirmChoice;
    std::cout << "确认支付并完成订单吗? (y/n): ";
    std::cin >> confirmChoice;
    m_ui->clearInputBuffer();

    if (tolower(confirmChoice) != 'y')
    {
        std::cout << "订单已取消。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 提交结算请求
    std::cout << "正在处理订单，请稍候..." << std::endl;
    std::string checkoutRequest = "CHECKOUT|" + m_currentUsername;
    std::string checkoutResponse;

    if (m_connection->sendRequest(checkoutRequest, checkoutResponse))
    {
        std::istringstream checkoutIss(checkoutResponse);
        std::string checkoutStatus, checkoutMessage, orderId;

        std::getline(checkoutIss, checkoutStatus, '|');
        std::getline(checkoutIss, checkoutMessage, '|');
        if (checkoutStatus == "CHECKOUT_SUCCESS")
        {
            std::getline(checkoutIss, orderId, '|');
            std::cout << "结算成功: " << checkoutMessage << std::endl;
            std::cout << "订单ID: " << orderId << std::endl;
        }
        else if (checkoutStatus == "CHECKOUT_PENDING")
        {
            std::getline(checkoutIss, orderId, '|');
            std::cout << "订单已提交但正在处理中: " << checkoutMessage << std::endl;
            std::cout << "订单ID: " << orderId << std::endl;
            std::cout << "您可以稍后在订单记录中查看订单状态。" << std::endl;
        }
        else
        {
            std::cout << "结算失败: " << checkoutMessage << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void ClientApp::handleLogout()
{
    std::string request = "LOGOUT|" + m_currentUsername;
    std::string response;

    if (m_connection->sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status;
        std::getline(iss, status, '|');

        if (status == "LOGOUT_SUCCESS")
        {
            std::cout << "已成功登出。" << std::endl;
            m_currentUsername = "";
            m_currentUserType = "";
            m_isLoggedIn = false;
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "登出失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }
}

void ClientApp::handleViewUserInfo()
{
    if (!m_isLoggedIn)
    {
        std::cout << "您尚未登录。" << std::endl;
        return;
    }

    std::string request = "GET_USER_INFO|" + m_currentUsername;
    std::string response;

    if (m_connection->sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, username, userType, balanceStr;

        std::getline(iss, status, '|');
        if (status == "USER_INFO")
        {
            std::getline(iss, username, '|');
            std::getline(iss, userType, '|');
            std::getline(iss, balanceStr, '|');

            double balance = std::stod(balanceStr);

            std::cout << "\n===== 用户信息 =====\n";
            std::cout << "用户名: " << username << std::endl;
            std::cout << "账户类型: " << userType << std::endl;
            std::cout << "账户余额: " << std::fixed << std::setprecision(2) << balance << " 元" << std::endl;
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "获取用户信息失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void ClientApp::handleCheckBalance()
{
    if (!m_isLoggedIn)
    {
        std::cout << "您尚未登录。" << std::endl;
        return;
    }

    std::string request = "CHECK_BALANCE|" + m_currentUsername;
    std::string response;

    if (m_connection->sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, balanceStr;

        std::getline(iss, status, '|');
        if (status == "BALANCE")
        {
            std::getline(iss, balanceStr, '|');
            double balance = std::stod(balanceStr);

            std::cout << "\n===== 账户余额 =====\n";
            std::cout << "当前余额: " << std::fixed << std::setprecision(2) << balance << " 元" << std::endl;
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "查询余额失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void ClientApp::handleDeposit()
{
    if (!m_isLoggedIn)
    {
        std::cout << "您尚未登录。" << std::endl;
        return;
    }

    double amount;
    std::cout << "\n===== 账户充值 =====\n";
    std::cout << "请输入充值金额: ";
    if (!(std::cin >> amount) || amount <= 0)
    {
        std::cout << "无效金额。" << std::endl;
        m_ui->clearInputBuffer();
        return;
    }
    m_ui->clearInputBuffer();

    std::string request = "DEPOSIT|" + m_currentUsername + "|" + std::to_string(amount);
    std::string response;

    if (m_connection->sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, message, balanceStr;

        std::getline(iss, status, '|');
        std::getline(iss, message, '|');

        if (status == "DEPOSIT_SUCCESS")
        {
            std::getline(iss, balanceStr, '|');
            double balance = std::stod(balanceStr);

            std::cout << message << std::endl;
            std::cout << "当前余额: " << std::fixed << std::setprecision(2) << balance << " 元" << std::endl;
        }
        else
        {
            std::cout << "充值失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void ClientApp::handleEnterStore()
{
    int choice = -1;

    do
    {
        std::cout << "\n===== 电子商城 =====\n";
        std::cout << "1. 浏览所有商品\n";
        std::cout << "2. 查看商品详情\n";
        std::cout << "3. 搜索商品\n";
        std::cout << "4. 添加商品到购物车\n";
        std::cout << "5. 直接购买商品\n";
        std::cout << "0. 返回主菜单\n";
        std::cout << "请选择: ";

        std::cin >> choice;
        m_ui->clearInputBuffer();

        switch (choice)
        {
        case 1:
            handleBrowseProducts();
            break;
        case 2:
            handleProductDetail();
            break;
        case 3:
            handleSearchProducts();
            break;
        case 4:
            handleAddToCart();
            break;
        case 5:
            handleDirectPurchase();
            break;
        case 0:
            std::cout << "返回主菜单。" << std::endl;
            break;
        default:
            std::cout << "无效选择，请重试。" << std::endl;
        }
    } while (choice != 0);
}

void ClientApp::handleSearchProducts()
{
    std::string searchTerm;
    std::cout << "\n===== 搜索商品 =====\n";
    std::cout << "输入搜索关键词: ";
    std::getline(std::cin, searchTerm);

    std::string request = "SEARCH_PRODUCTS|" + searchTerm;
    std::string response;

    if (m_connection->sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, countStr;

        std::getline(iss, status, '|');
        std::getline(iss, countStr, '|');

        if (status == "SEARCH_RESULTS")
        {
            int count = std::stoi(countStr);

            if (count == 0)
            {
                std::cout << "没有找到匹配的商品。" << std::endl;
            }
            else
            {
                std::cout << "找到 " << count << " 个匹配商品:\n";
                std::cout << std::left << std::setw(20) << "商品名称" << std::setw(10) << "类型" << std::setw(10) << "价格" << std::setw(10) << "库存" << std::setw(15) << "商家" << std::endl;
                std::cout << std::string(65, '-') << std::endl;

                for (int i = 0; i < count; i++)
                {
                    std::string productInfo;
                    std::getline(iss, productInfo, '|');

                    std::istringstream productIss(productInfo);
                    std::string name, type, price, quantity, seller;

                    std::getline(productIss, name, ',');
                    std::getline(productIss, type, ',');
                    std::getline(productIss, price, ',');
                    std::getline(productIss, quantity, ',');
                    std::getline(productIss, seller, ',');

                    std::cout << std::left << std::setw(20) << name << std::setw(10) << type << std::setw(10) << price << std::setw(10) << quantity << std::setw(15) << seller << std::endl;
                }
            }
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "搜索失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void ClientApp::handleDirectPurchase()
{
    if (!m_isLoggedIn || m_currentUserType != "customer")
    {
        std::cout << "请先以消费者身份登录。" << std::endl;
        return;
    }

    std::string productName, sellerName;
    int quantity;

    std::cout << "\n===== 直接购买商品 =====\n";
    std::cout << "输入商品名称: ";
    std::getline(std::cin, productName);
    std::cout << "输入商家名称: ";
    std::getline(std::cin, sellerName);
    std::cout << "输入数量: ";
    if (!(std::cin >> quantity) || quantity <= 0)
    {
        std::cout << "无效数量。" << std::endl;
        m_ui->clearInputBuffer();
        return;
    }
    m_ui->clearInputBuffer();

    // 首先获取商品详情和价格
    std::string detailRequest = "GET_PRODUCT_DETAIL|" + productName + "|" + sellerName;
    std::string detailResponse;
    double price = 0.0;
    int stock = 0;

    if (m_connection->sendRequest(detailRequest, detailResponse))
    {
        std::istringstream detailIss(detailResponse);
        std::string detailStatus;
        std::getline(detailIss, detailStatus, '|');

        if (detailStatus == "PRODUCT_DETAIL")
        {
            std::string name, type, description, priceStr, stockStr, seller;
            std::getline(detailIss, name, '|');
            std::getline(detailIss, type, '|');
            std::getline(detailIss, description, '|');
            std::getline(detailIss, priceStr, '|');
            std::getline(detailIss, stockStr, '|');

            price = std::stod(priceStr);
            stock = std::stoi(stockStr);

            if (stock < quantity)
            {
                std::cout << "库存不足。当前库存: " << stock << std::endl;
                std::cout << "按回车键继续...";
                std::cin.get();
                return;
            }
        }
        else
        {
            std::string message;
            std::getline(detailIss, message, '|');
            std::cout << "获取商品信息失败: " << message << std::endl;
            std::cout << "按回车键继续...";
            std::cin.get();
            return;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 获取用户余额
    std::string balanceRequest = "CHECK_BALANCE|" + m_currentUsername;
    std::string balanceResponse;
    double balance = 0.0;

    if (m_connection->sendRequest(balanceRequest, balanceResponse))
    {
        std::istringstream balanceIss(balanceResponse);
        std::string balanceStatus, balanceStr;
        std::getline(balanceIss, balanceStatus, '|');

        if (balanceStatus == "BALANCE")
        {
            std::getline(balanceIss, balanceStr, '|');
            balance = std::stod(balanceStr);
        }
        else
        {
            std::string message;
            std::getline(balanceIss, message, '|');
            std::cout << "获取余额失败: " << message << std::endl;
            std::cout << "按回车键继续...";
            std::cin.get();
            return;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 计算总价并确认购买
    double totalCost = price * quantity;
    std::cout << "\n订单预览:" << std::endl;
    std::cout << "商品: " << productName << " (卖家: " << sellerName << ")" << std::endl;
    std::cout << "数量: " << quantity << std::endl;
    std::cout << "单价: ¥" << std::fixed << std::setprecision(2) << price << std::endl;
    std::cout << "总价: ¥" << std::fixed << std::setprecision(2) << totalCost << std::endl;
    std::cout << "当前余额: ¥" << std::fixed << std::setprecision(2) << balance << std::endl;

    if (balance < totalCost)
    {
        std::cout << "余额不足，无法完成购买。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    char confirm;
    std::cout << "确认购买? (y/n): ";
    std::cin >> confirm;
    m_ui->clearInputBuffer();

    if (tolower(confirm) != 'y')
    {
        std::cout << "购买已取消。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 发送直接购买请求
    std::string purchaseRequest = "DIRECT_PURCHASE|" + m_currentUsername + "|" + productName + "|" + sellerName + "|" + std::to_string(quantity);
    std::string purchaseResponse;

    if (m_connection->sendRequest(purchaseRequest, purchaseResponse))
    {
        std::istringstream purchaseIss(purchaseResponse);
        std::string purchaseStatus, purchaseMessage, orderIdStr;
        std::getline(purchaseIss, purchaseStatus, '|');
        std::getline(purchaseIss, purchaseMessage, '|');

        if (purchaseStatus == "PURCHASE_SUCCESS")
        {
            std::getline(purchaseIss, orderIdStr, '|');
            std::cout << "购买成功: " << purchaseMessage << std::endl;
            std::cout << "订单编号: " << orderIdStr << std::endl;
        }
        else
        {
            std::cout << "购买失败: " << purchaseMessage << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void ClientApp::handleSellerInfo()
{
    if (!m_isLoggedIn || m_currentUserType != "seller")
    {
        std::cout << "请先以商家身份登录。" << std::endl;
        return;
    }

    std::string request = "GET_SELLER_INFO|" + m_currentUsername;
    std::string response;

    if (m_connection->sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, username, incomeStr, productCountStr;

        std::getline(iss, status, '|');

        if (status == "SELLER_INFO")
        {
            std::string username, userType, balanceStr;

            std::getline(iss, username, '|');
            std::getline(iss, incomeStr, '|');
            std::getline(iss, productCountStr, '|');

            double income = std::stod(incomeStr);
            int productCount = std::stoi(productCountStr);

            std::cout << "\n===== 商家信息 =====\n";
            std::cout << "用户名: " << username << "\n";
            std::cout << "账户类型: " << userType << "\n";
            std::cout << "账户余额: " << balanceStr << " 元\n";
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "获取商家信息失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void ClientApp::handleChangePassword()
{
    if (!m_isLoggedIn)
    {
        std::cout << "您尚未登录。" << std::endl;
        return;
    }

    std::string oldPassword, newPassword, confirmPassword;

    std::cout << "\n===== 修改密码 =====\n";
    std::cout << "请输入当前密码: ";
    std::getline(std::cin, oldPassword);
    std::cout << "请输入新密码: ";
    std::getline(std::cin, newPassword);
    std::cout << "请确认新密码: ";
    std::getline(std::cin, confirmPassword);

    if (newPassword != confirmPassword)
    {
        std::cout << "两次输入的新密码不一致。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    std::string request = "CHANGE_PASSWORD|" + m_currentUsername + "|" + oldPassword + "|" + newPassword;
    std::string response;

    if (m_connection->sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, message;

        std::getline(iss, status, '|');
        std::getline(iss, message, '|');

        if (status == "PASSWORD_CHANGED")
        {
            std::cout << "密码修改成功: " << message << std::endl;
        }
        else
        {
            std::cout << "密码修改失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void ClientApp::handleManageProducts()
{
    if (!m_isLoggedIn || m_currentUserType != "seller")
    {
        std::cout << "请先以商家身份登录。" << std::endl;
        return;
    }

    int choice = -1;
    do
    {
        std::cout << "\n===== 商品管理 =====\n";
        std::cout << "1. 查看我的商品\n";
        std::cout << "2. 添加新商品\n";
        std::cout << "3. 修改商品\n";
        std::cout << "4. 下架商品\n";
        std::cout << "0. 返回主菜单\n";
        std::cout << "请选择: ";

        if (!(std::cin >> choice))
        {
            std::cout << "无效输入。" << std::endl;
            m_ui->clearInputBuffer();
            continue;
        }
        m_ui->clearInputBuffer();

        switch (choice)
        {
        case 1:
            viewMyProducts();
            break;
        case 2:
            addNewProduct();
            break;
        case 3:
            modifyProduct();
            break;
        case 4:
            removeProduct();
            break;
        case 0:
            std::cout << "返回主菜单。" << std::endl;
            break;
        default:
            std::cout << "无效选择，请重试。" << std::endl;
        }
    } while (choice != 0);
}

void ClientApp::viewMyProducts()
{
    std::string request = "GET_MY_PRODUCTS|" + m_currentUsername;
    std::string response;

    if (m_connection->sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, countStr;

        std::getline(iss, status, '|');
        std::getline(iss, countStr, '|');

        if (status == "MY_PRODUCTS")
        {
            int count = std::stoi(countStr);

            if (count == 0)
            {
                std::cout << "您目前没有在售的商品。" << std::endl;
            }
            else
            {
                std::cout << "\n===== 我的商品列表 =====\n";
                std::cout << std::left << std::setw(20) << "商品名称" << std::setw(10) << "类型" << std::setw(10) << "价格" << std::setw(10) << "库存" << std::setw(15) << "销量" << std::endl;
                std::cout << std::string(65, '-') << std::endl;

                for (int i = 0; i < count; i++)
                {
                    std::string productInfo;
                    std::getline(iss, productInfo, '|');

                    std::istringstream productIss(productInfo);
                    std::string name, type, price, quantity, sales;

                    std::getline(productIss, name, ',');
                    std::getline(productIss, type, ',');
                    std::getline(productIss, price, ',');
                    std::getline(productIss, quantity, ',');
                    std::getline(productIss, sales, ',');

                    std::cout << std::left << std::setw(20) << name << std::setw(10) << type << std::setw(10) << price << std::setw(10) << quantity << std::setw(15) << sales << std::endl;
                }
            }
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "获取商品列表失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void ClientApp::addNewProduct()
{
    std::string name, type, description, priceStr, quantityStr;
    std::map<std::string, std::string> additionalProperties;

    std::cout << "\n===== 添加新商品 =====\n";
    std::cout << "商品名称: ";
    std::getline(std::cin, name);

    if (name.empty())
    {
        std::cout << "商品名称不能为空。" << std::endl;
        return;
    }

    std::cout << "商品类型 (Book, Clothing, Food, Electronics, Other): ";
    std::getline(std::cin, type);

    std::cout << "商品描述: ";
    std::getline(std::cin, description);

    double price;
    std::cout << "商品价格: ";
    if (!(std::cin >> price) || price <= 0)
    {
        std::cout << "无效的价格输入。" << std::endl;
        m_ui->clearInputBuffer();
        return;
    }
    priceStr = std::to_string(price);
    m_ui->clearInputBuffer();

    int quantity;
    std::cout << "初始库存: ";
    if (!(std::cin >> quantity) || quantity < 0)
    {
        std::cout << "无效的库存输入。" << std::endl;
        m_ui->clearInputBuffer();
        return;
    }
    quantityStr = std::to_string(quantity);
    m_ui->clearInputBuffer();

    // 根据不同的商品类型收集额外属性
    if (type == "Book")
    {
        std::cout << "作者: ";
        std::getline(std::cin, additionalProperties["author"]);
        std::cout << "ISBN: ";
        std::getline(std::cin, additionalProperties["isbn"]);
    }
    else if (type == "Clothing")
    {
        std::cout << "尺寸: ";
        std::getline(std::cin, additionalProperties["size"]);
        std::cout << "颜色: ";
        std::getline(std::cin, additionalProperties["color"]);
    }
    else if (type == "Food")
    {
        std::cout << "保质期 (YYYY-MM-DD): ";
        std::getline(std::cin, additionalProperties["expDate"]);
    }
    else if (type == "Electronics")
    {
        std::cout << "品牌: ";
        std::getline(std::cin, additionalProperties["brand"]);
        std::cout << "保修期(月): ";
        std::getline(std::cin, additionalProperties["warranty"]);
    }

    // 构建请求
    std::string request = "ADD_PRODUCT|" + m_currentUsername + "|" + name + "|" + type + "|" + description + "|" + priceStr + "|" + quantityStr;

    // 添加额外属性
    for (const auto &prop : additionalProperties)
    {
        request += "|" + prop.first + ":" + prop.second;
    }

    std::string response;
    if (m_connection->sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, message;

        std::getline(iss, status, '|');
        std::getline(iss, message, '|');

        if (status == "PRODUCT_ADDED")
        {
            std::cout << "商品添加成功: " << message << std::endl;
        }
        else
        {
            std::cout << "商品添加失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void ClientApp::modifyProduct()
{
    std::string productName;
    std::cout << "\n===== 修改商品 =====\n";

    // 先获取当前商品列表
    viewMyProducts();

    std::cout << "请输入要修改的商品名称: ";
    std::getline(std::cin, productName);

    if (productName.empty())
    {
        std::cout << "商品名称不能为空。" << std::endl;
        return;
    }

    // 获取商品详情
    std::string detailRequest = "GET_PRODUCT_DETAIL|" + productName + "|" + m_currentUsername;
    std::string detailResponse;

    if (!m_connection->sendRequest(detailRequest, detailResponse))
    {
        std::cout << "与服务器通信失败。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    std::istringstream iss(detailResponse);
    std::string status;
    std::getline(iss, status, '|');

    if (status != "PRODUCT_DETAIL")
    {
        std::string message;
        std::getline(iss, message, '|');
        std::cout << "获取商品详情失败: " << message << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 显示可修改的属性
    std::cout << "\n请选择要修改的属性:\n";
    std::cout << "1. 商品描述\n";
    std::cout << "2. 商品价格\n";
    std::cout << "3. 商品库存\n";
    std::cout << "0. 取消\n";
    std::cout << "请选择: ";

    int choice;
    if (!(std::cin >> choice) || choice < 0 || choice > 3)
    {
        std::cout << "无效选择。" << std::endl;
        m_ui->clearInputBuffer();
        return;
    }
    m_ui->clearInputBuffer();

    if (choice == 0)
    {
        std::cout << "已取消修改。" << std::endl;
        return;
    }

    std::string field, newValue;

    switch (choice)
    {
    case 1:
        field = "description";
        std::cout << "输入新的商品描述: ";
        std::getline(std::cin, newValue);
        break;
    case 2:
        field = "price";
        double price;
        std::cout << "输入新的商品价格: ";
        if (!(std::cin >> price) || price <= 0)
        {
            std::cout << "无效的价格输入。" << std::endl;
            m_ui->clearInputBuffer();
            return;
        }
        newValue = std::to_string(price);
        m_ui->clearInputBuffer();
        break;
    case 3:
        field = "quantity";
        int quantity;
        std::cout << "输入新的商品库存: ";
        if (!(std::cin >> quantity) || quantity < 0)
        {
            std::cout << "无效的库存输入。" << std::endl;
            m_ui->clearInputBuffer();
            return;
        }
        newValue = std::to_string(quantity);
        m_ui->clearInputBuffer();
        break;
    }

    // 发送修改请求
    std::string updateRequest = "UPDATE_PRODUCT|" + m_currentUsername + "|" + productName + "|" + field + "|" + newValue;
    std::string updateResponse;

    if (m_connection->sendRequest(updateRequest, updateResponse))
    {
        std::istringstream updateIss(updateResponse);
        std::string updateStatus, updateMessage;

        std::getline(updateIss, updateStatus, '|');
        std::getline(updateIss, updateMessage, '|');

        if (updateStatus == "PRODUCT_UPDATED")
        {
            std::cout << "商品更新成功: " << updateMessage << std::endl;
        }
        else
        {
            std::cout << "商品更新失败: " << updateMessage << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void ClientApp::removeProduct()
{
    std::string productName;
    std::cout << "\n===== 下架商品 =====\n";

    // 先获取当前商品列表
    viewMyProducts();

    std::cout << "请输入要下架的商品名称: ";
    std::getline(std::cin, productName);

    if (productName.empty())
    {
        std::cout << "商品名称不能为空。" << std::endl;
        return;
    }

    char confirm;
    std::cout << "确认下架商品 '" << productName << "'? (y/n): ";
    std::cin >> confirm;
    m_ui->clearInputBuffer();

    if (tolower(confirm) != 'y')
    {
        std::cout << "已取消下架操作。" << std::endl;
        std::cout << "按回车键继续...";
        std::cin.get();
        return;
    }

    // 发送下架请求
    std::string removeRequest = "REMOVE_PRODUCT|" + m_currentUsername + "|" + productName;
    std::string removeResponse;

    if (m_connection->sendRequest(removeRequest, removeResponse))
    {
        std::istringstream removeIss(removeResponse);
        std::string removeStatus, removeMessage;

        std::getline(removeIss, removeStatus, '|');
        std::getline(removeIss, removeMessage, '|');

        if (removeStatus == "PRODUCT_REMOVED")
        {
            std::cout << "商品下架成功: " << removeMessage << std::endl;
        }
        else
        {
            std::cout << "商品下架失败: " << removeMessage << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void ClientApp::handleCheckIncome()
{
    if (!m_isLoggedIn || m_currentUserType != "seller")
    {
        std::cout << "请先以商家身份登录。" << std::endl;
        return;
    }

    std::string request = "CHECK_INCOME|" + m_currentUsername;
    std::string response;

    if (m_connection->sendRequest(request, response))
    {
        std::istringstream iss(response);
        std::string status, totalIncomeStr, periodIncomeStr, orderCountStr;

        std::getline(iss, status, '|');
        if (status == "INCOME_INFO")
        {
            std::getline(iss, totalIncomeStr, '|');
            std::getline(iss, periodIncomeStr, '|');
            std::getline(iss, orderCountStr, '|');

            try
            {
                double totalIncome = std::stod(totalIncomeStr);
                double periodIncome = std::stod(periodIncomeStr);
                int orderCount = std::stoi(orderCountStr);

                std::cout << "\n===== 收入统计 =====\n";
                std::cout << "总收入: " << std::fixed << std::setprecision(2) << totalIncome << " 元" << std::endl;
                std::cout << "本月收入: " << std::fixed << std::setprecision(2) << periodIncome << " 元" << std::endl;
                std::cout << "完成订单数: " << orderCount << std::endl;

                // 显示收入趋势
                if (orderCount > 0)
                {
                    std::cout << "\n--- 近期销售情况 ---\n";
                    int displayCount = 5; // 最多显示5条
                    int count = std::min(displayCount, orderCount);

                    std::cout << "最近 " << count << " 个订单:\n";
                    std::cout << std::left << std::setw(15) << "订单ID" << std::setw(15) << "日期" << std::setw(10) << "金额" << std::setw(15) << "客户" << std::endl;
                    std::cout << std::string(55, '-') << std::endl;

                    for (int i = 0; i < count; i++)
                    {
                        std::string orderInfo;
                        std::getline(iss, orderInfo, '|');

                        std::istringstream orderIss(orderInfo);
                        std::string id, date, amount, customer;

                        std::getline(orderIss, id, ',');
                        std::getline(orderIss, date, ',');
                        std::getline(orderIss, amount, ',');
                        std::getline(orderIss, customer, ',');

                        std::cout << std::left << std::setw(15) << id << std::setw(15) << date << std::setw(10) << amount << std::setw(15) << customer << std::endl;
                    }
                }
            }
            catch (const std::invalid_argument &e)
            {
                std::cout << "解析数据时出错: " << e.what() << std::endl;
            }
            catch (const std::exception &e)
            {
                std::cout << "发生错误: " << e.what() << std::endl;
            }
        }
        else
        {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "获取收入信息失败: " << message << std::endl;
        }
    }
    else
    {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}
