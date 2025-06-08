#include "client_ui.h"
#include <iostream>
#include <iomanip>
#include <limits>

ClientUI::ClientUI(const std::string &serverHost, int serverPort)
    : api(serverHost, serverPort), isRunning(false)
{
}

ClientUI::~ClientUI()
{
    stop();
}

void ClientUI::run()
{
    isRunning = true;

    // 连接到服务器
    if (!api.connect())
    {
        std::cout << "Failed to connect to server. Please check if the server is running." << std::endl;
        pause();
        return;
    }

    std::cout << "Connected to server successfully." << std::endl;

    // 主循环
    while (isRunning)
    {
        showMainMenu();
        handleMainMenu();
    }
}

void ClientUI::stop()
{
    isRunning = false;
    api.disconnect();
}

void ClientUI::showHeader(const std::string &title)
{
    system("cls"); // 清屏
    std::cout << "===============================================" << std::endl;
    std::cout << "      电子商务系统 - " << title << std::endl;
    std::cout << "===============================================" << std::endl;

    if (api.isAuthenticated())
    {
        std::cout << "欢迎, " << api.getUsername() << " (" << api.getUserType() << ")" << std::endl;
    }

    std::cout << std::endl;
}

void ClientUI::showFooter()
{
    std::cout << std::endl;
    std::cout << "===============================================" << std::endl;
}

void ClientUI::pause()
{
    std::cout << "\n按任意键继续...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

int ClientUI::getMenuChoice(int min, int max)
{
    int choice;
    bool valid = false;

    do
    {
        std::cout << "请选择 (" << min << "-" << max << "): ";
        std::cin >> choice;

        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "无效输入，请重试。" << std::endl;
        }
        else if (choice < min || choice > max)
        {
            std::cout << "选择超出范围，请重试。" << std::endl;
        }
        else
        {
            valid = true;
        }
    } while (!valid);

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return choice;
}

std::string ClientUI::getInput(const std::string &prompt)
{
    std::string input;
    std::cout << prompt;
    std::getline(std::cin, input);
    return input;
}

void ClientUI::showMainMenu()
{
    showHeader("主菜单");

    std::cout << "1. 产品管理" << std::endl;

    if (api.isAuthenticated())
    {
        std::cout << "2. 购物车管理" << std::endl;
        std::cout << "3. 订单管理" << std::endl;
        std::cout << "4. 退出登录" << std::endl;
        std::cout << "0. 退出程序" << std::endl;
    }
    else
    {
        std::cout << "2. 登录" << std::endl;
        std::cout << "3. 注册" << std::endl;
        std::cout << "0. 退出程序" << std::endl;
    }

    showFooter();
}

void ClientUI::handleMainMenu()
{
    int choice;

    if (api.isAuthenticated())
    {
        choice = getMenuChoice(0, 4);

        switch (choice)
        {
        case 1:
            showProductMenu();
            break;
        case 2:
            showCartMenu();
            break;
        case 3:
            showOrderMenu();
            break;
        case 4:
            handleLogout();
            break;
        case 0:
            isRunning = false;
            break;
        }
    }
    else
    {
        choice = getMenuChoice(0, 3);

        switch (choice)
        {
        case 1:
            showProductMenu();
            break;
        case 2:
            showLoginMenu();
            break;
        case 3:
            handleRegister();
            break;
        case 0:
            isRunning = false;
            break;
        }
    }
}

void ClientUI::showLoginMenu()
{
    showHeader("登录");

    handleLogin();

    showFooter();
    pause();
}

void ClientUI::handleLogin()
{
    std::string username = getInput("用户名: ");
    std::string password = getInput("密码: ");

    if (api.login(username, password))
    {
        std::cout << "登录成功！" << std::endl;
    }
    else
    {
        std::cout << "登录失败，用户名或密码错误。" << std::endl;
    }
}

void ClientUI::handleRegister()
{
    showHeader("注册");

    std::string username = getInput("用户名: ");
    std::string password = getInput("密码: ");

    std::cout << "用户类型:" << std::endl;
    std::cout << "1. 消费者" << std::endl;
    std::cout << "2. 卖家" << std::endl;

    int choice = getMenuChoice(1, 2);
    std::string userType = (choice == 1) ? "customer" : "seller";

    if (api.registerUser(username, password, userType))
    {
        std::cout << "注册成功并自动登录！" << std::endl;
    }
    else
    {
        std::cout << "注册失败，用户名可能已被使用。" << std::endl;
    }

    showFooter();
    pause();
}

void ClientUI::handleLogout()
{
    if (api.logout())
    {
        std::cout << "已成功退出登录。" << std::endl;
    }
    else
    {
        std::cout << "退出登录失败。" << std::endl;
    }

    pause();
}

void ClientUI::showProductMenu()
{
    showHeader("产品管理");

    std::cout << "1. 浏览所有产品" << std::endl;
    std::cout << "2. 搜索产品" << std::endl;
    std::cout << "3. 查看产品详情" << std::endl;
    std::cout << "0. 返回主菜单" << std::endl;

    showFooter();

    int choice = getMenuChoice(0, 3);

    switch (choice)
    {
    case 1:
        handleProductList();
        break;
    case 2:
        handleProductSearch();
        break;
    case 3:
    {
        std::string productIdStr = getInput("请输入产品ID: ");
        try
        {
            int productId = std::stoi(productIdStr);
            handleProductDetail(productId);
        }
        catch (const std::exception &e)
        {
            std::cout << "无效的产品ID。" << std::endl;
            pause();
        }
    }
    break;
    case 0:
        return;
    }
}

void ClientUI::handleProductList()
{
    showHeader("所有产品");

    auto products = api.getProducts();

    if (products.empty())
    {
        std::cout << "没有找到产品。" << std::endl;
    }
    else
    {
        std::cout << std::left << std::setw(5) << "ID"
                  << std::setw(20) << "名称"
                  << std::setw(10) << "价格"
                  << std::setw(20) << "卖家" << std::endl;
        std::cout << "---------------------------------------------------------------" << std::endl;

        for (const auto &product : products)
        {
            std::cout << std::left << std::setw(5) << product.id
                      << std::setw(20) << product.name
                      << std::setw(10) << product.price
                      << std::setw(20) << product.seller << std::endl;
        }
    }

    showFooter();
    pause();
}

void ClientUI::handleProductSearch()
{
    showHeader("搜索产品");

    std::string keyword = getInput("请输入搜索关键词: ");

    auto products = api.searchProducts(keyword);

    std::cout << "搜索结果：" << std::endl
              << std::endl;

    if (products.empty())
    {
        std::cout << "没有找到匹配的产品。" << std::endl;
    }
    else
    {
        std::cout << std::left << std::setw(5) << "ID"
                  << std::setw(20) << "名称"
                  << std::setw(10) << "价格"
                  << std::setw(20) << "卖家" << std::endl;
        std::cout << "---------------------------------------------------------------" << std::endl;

        for (const auto &product : products)
        {
            std::cout << std::left << std::setw(5) << product.id
                      << std::setw(20) << product.name
                      << std::setw(10) << product.price
                      << std::setw(20) << product.seller << std::endl;
        }
    }

    showFooter();
    pause();
}

void ClientUI::handleProductDetail(int productId)
{
    showHeader("产品详情");

    auto product = api.getProductDetail(productId);

    if (product.id == 0)
    {
        std::cout << "没有找到该产品。" << std::endl;
    }
    else
    {
        std::cout << "ID: " << product.id << std::endl;
        std::cout << "名称: " << product.name << std::endl;
        std::cout << "价格: " << product.price << std::endl;
        std::cout << "卖家: " << product.seller << std::endl;
        std::cout << "描述: " << product.description << std::endl;

        // 如果用户已登录，显示添加到购物车选项
        if (api.isAuthenticated() && api.getUserType() == "customer")
        {
            std::cout << std::endl;
            std::cout << "1. 添加到购物车" << std::endl;
            std::cout << "0. 返回" << std::endl;

            int choice = getMenuChoice(0, 1);

            if (choice == 1)
            {
                std::string quantityStr = getInput("请输入数量: ");
                try
                {
                    int quantity = std::stoi(quantityStr);
                    if (quantity > 0)
                    {
                        if (api.addToCart(productId, quantity))
                        {
                            std::cout << "成功添加到购物车！" << std::endl;
                        }
                        else
                        {
                            std::cout << "添加到购物车失败。" << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "数量必须大于0。" << std::endl;
                    }
                }
                catch (const std::exception &e)
                {
                    std::cout << "无效的数量。" << std::endl;
                }
            }
        }
    }

    showFooter();
    pause();
}

void ClientUI::showCartMenu()
{
    if (!api.isAuthenticated() || api.getUserType() != "customer")
    {
        std::cout << "您必须以客户身份登录才能访问购物车。" << std::endl;
        pause();
        return;
    }

    showHeader("购物车管理");

    std::cout << "1. 查看购物车" << std::endl;
    std::cout << "2. 添加商品到购物车" << std::endl;
    std::cout << "3. 更新购物车商品数量" << std::endl;
    std::cout << "4. 从购物车移除商品" << std::endl;
    std::cout << "5. 结算购物车" << std::endl;
    std::cout << "0. 返回主菜单" << std::endl;

    showFooter();

    int choice = getMenuChoice(0, 5);

    switch (choice)
    {
    case 1:
        handleCartList();
        break;
    case 2:
        handleCartAdd();
        break;
    case 3:
        handleCartUpdate();
        break;
    case 4:
        handleCartRemove();
        break;
    case 5:
        handleOrderCreate();
        break;
    case 0:
        return;
    }
}

void ClientUI::handleCartList()
{
    showHeader("购物车");

    auto cartItems = api.getCart();

    if (cartItems.empty())
    {
        std::cout << "购物车为空。" << std::endl;
    }
    else
    {
        double totalAmount = 0.0;

        std::cout << std::left << std::setw(5) << "ID"
                  << std::setw(20) << "名称"
                  << std::setw(10) << "价格"
                  << std::setw(10) << "数量"
                  << std::setw(10) << "小计" << std::endl;
        std::cout << "---------------------------------------------------------------" << std::endl;

        for (const auto &item : cartItems)
        {
            std::cout << std::left << std::setw(5) << item.productId
                      << std::setw(20) << item.name
                      << std::setw(10) << item.price
                      << std::setw(10) << item.quantity
                      << std::setw(10) << item.totalPrice << std::endl;

            totalAmount += item.totalPrice;
        }

        std::cout << "---------------------------------------------------------------" << std::endl;
        std::cout << "总金额: " << totalAmount << std::endl;
    }

    showFooter();
    pause();
}

void ClientUI::handleCartAdd()
{
    showHeader("添加商品到购物车");

    // 先展示所有产品
    auto products = api.getProducts();

    if (products.empty())
    {
        std::cout << "没有找到产品。" << std::endl;
    }
    else
    {
        std::cout << std::left << std::setw(5) << "ID"
                  << std::setw(20) << "名称"
                  << std::setw(10) << "价格"
                  << std::setw(20) << "卖家" << std::endl;
        std::cout << "---------------------------------------------------------------" << std::endl;

        for (const auto &product : products)
        {
            std::cout << std::left << std::setw(5) << product.id
                      << std::setw(20) << product.name
                      << std::setw(10) << product.price
                      << std::setw(20) << product.seller << std::endl;
        }

        std::cout << "---------------------------------------------------------------" << std::endl;

        std::string productIdStr = getInput("请输入要添加的产品ID: ");
        try
        {
            int productId = std::stoi(productIdStr);

            std::string quantityStr = getInput("请输入数量: ");
            try
            {
                int quantity = std::stoi(quantityStr);
                if (quantity > 0)
                {
                    if (api.addToCart(productId, quantity))
                    {
                        std::cout << "成功添加到购物车！" << std::endl;
                    }
                    else
                    {
                        std::cout << "添加到购物车失败。" << std::endl;
                    }
                }
                else
                {
                    std::cout << "数量必须大于0。" << std::endl;
                }
            }
            catch (const std::exception &e)
            {
                std::cout << "无效的数量。" << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cout << "无效的产品ID。" << std::endl;
        }
    }

    showFooter();
    pause();
}

void ClientUI::handleCartUpdate()
{
    showHeader("更新购物车商品数量");

    auto cartItems = api.getCart();

    if (cartItems.empty())
    {
        std::cout << "购物车为空。" << std::endl;
    }
    else
    {
        std::cout << std::left << std::setw(5) << "ID"
                  << std::setw(20) << "名称"
                  << std::setw(10) << "价格"
                  << std::setw(10) << "数量"
                  << std::setw(10) << "小计" << std::endl;
        std::cout << "---------------------------------------------------------------" << std::endl;

        for (const auto &item : cartItems)
        {
            std::cout << std::left << std::setw(5) << item.productId
                      << std::setw(20) << item.name
                      << std::setw(10) << item.price
                      << std::setw(10) << item.quantity
                      << std::setw(10) << item.totalPrice << std::endl;
        }

        std::cout << "---------------------------------------------------------------" << std::endl;

        std::string productIdStr = getInput("请输入要更新的产品ID: ");
        try
        {
            int productId = std::stoi(productIdStr);

            // 检查产品是否在购物车中
            bool found = false;
            for (const auto &item : cartItems)
            {
                if (item.productId == productId)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                std::cout << "该产品不在购物车中。" << std::endl;
            }
            else
            {
                std::string quantityStr = getInput("请输入新的数量: ");
                try
                {
                    int quantity = std::stoi(quantityStr);
                    if (quantity > 0)
                    {
                        if (api.updateCartItemQuantity(productId, quantity))
                        {
                            std::cout << "成功更新购物车！" << std::endl;
                        }
                        else
                        {
                            std::cout << "更新购物车失败。" << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "数量必须大于0。如要移除商品，请使用移除功能。" << std::endl;
                    }
                }
                catch (const std::exception &e)
                {
                    std::cout << "无效的数量。" << std::endl;
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cout << "无效的产品ID。" << std::endl;
        }
    }

    showFooter();
    pause();
}

void ClientUI::handleCartRemove()
{
    showHeader("从购物车移除商品");

    auto cartItems = api.getCart();

    if (cartItems.empty())
    {
        std::cout << "购物车为空。" << std::endl;
    }
    else
    {
        std::cout << std::left << std::setw(5) << "ID"
                  << std::setw(20) << "名称"
                  << std::setw(10) << "价格"
                  << std::setw(10) << "数量"
                  << std::setw(10) << "小计" << std::endl;
        std::cout << "---------------------------------------------------------------" << std::endl;

        for (const auto &item : cartItems)
        {
            std::cout << std::left << std::setw(5) << item.productId
                      << std::setw(20) << item.name
                      << std::setw(10) << item.price
                      << std::setw(10) << item.quantity
                      << std::setw(10) << item.totalPrice << std::endl;
        }

        std::cout << "---------------------------------------------------------------" << std::endl;

        std::string productIdStr = getInput("请输入要移除的产品ID: ");
        try
        {
            int productId = std::stoi(productIdStr);

            // 检查产品是否在购物车中
            bool found = false;
            for (const auto &item : cartItems)
            {
                if (item.productId == productId)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                std::cout << "该产品不在购物车中。" << std::endl;
            }
            else
            {
                if (api.removeFromCart(productId))
                {
                    std::cout << "成功从购物车移除！" << std::endl;
                }
                else
                {
                    std::cout << "从购物车移除失败。" << std::endl;
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cout << "无效的产品ID。" << std::endl;
        }
    }

    showFooter();
    pause();
}

void ClientUI::showOrderMenu()
{
    if (!api.isAuthenticated())
    {
        std::cout << "您必须登录才能访问订单管理。" << std::endl;
        pause();
        return;
    }

    showHeader("订单管理");

    std::cout << "1. 查看所有订单" << std::endl;
    std::cout << "2. 查看订单详情" << std::endl;

    if (api.getUserType() == "customer")
    {
        std::cout << "3. 创建新订单" << std::endl;
        std::cout << "4. 支付订单" << std::endl;
        std::cout << "5. 取消订单" << std::endl;
    }

    std::cout << "0. 返回主菜单" << std::endl;

    showFooter();

    int maxChoice = (api.getUserType() == "customer") ? 5 : 2;
    int choice = getMenuChoice(0, maxChoice);

    switch (choice)
    {
    case 1:
        handleOrderList();
        break;
    case 2:
    {
        std::string orderIdStr = getInput("请输入订单ID: ");
        try
        {
            int orderId = std::stoi(orderIdStr);
            handleOrderDetail(orderId);
        }
        catch (const std::exception &e)
        {
            std::cout << "无效的订单ID。" << std::endl;
            pause();
        }
    }
    break;
    case 3:
        if (api.getUserType() == "customer")
        {
            handleOrderCreate();
        }
        break;
    case 4:
        if (api.getUserType() == "customer")
        {
            handleOrderPay();
        }
        break;
    case 5:
        if (api.getUserType() == "customer")
        {
            handleOrderCancel();
        }
        break;
    case 0:
        return;
    }
}

void ClientUI::handleOrderList()
{
    showHeader("订单列表");

    auto orders = api.getOrders();

    if (orders.empty())
    {
        std::cout << "没有找到订单。" << std::endl;
    }
    else
    {
        std::cout << std::left << std::setw(5) << "ID"
                  << std::setw(20) << "日期"
                  << std::setw(10) << "状态"
                  << std::setw(10) << "金额";

        if (api.getUserType() == "seller")
        {
            std::cout << std::setw(15) << "客户";
        }

        std::cout << std::endl;
        std::cout << "---------------------------------------------------------------" << std::endl;

        for (const auto &order : orders)
        {
            std::cout << std::left << std::setw(5) << order.id
                      << std::setw(20) << order.date
                      << std::setw(10) << order.status
                      << std::setw(10) << order.totalAmount;

            if (api.getUserType() == "seller")
            {
                std::cout << std::setw(15) << order.customer;
            }

            std::cout << std::endl;
        }
    }

    showFooter();
    pause();
}

void ClientUI::handleOrderDetail(int orderId)
{
    showHeader("订单详情");

    auto order = api.getOrderDetail(orderId);

    if (order.id == 0)
    {
        std::cout << "没有找到该订单。" << std::endl;
    }
    else
    {
        std::cout << "订单ID: " << order.id << std::endl;
        std::cout << "日期: " << order.date << std::endl;
        std::cout << "状态: " << order.status << std::endl;
        std::cout << "总金额: " << order.totalAmount << std::endl;

        if (api.getUserType() == "seller")
        {
            std::cout << "客户: " << order.customer << std::endl;
        }

        std::cout << std::endl;
        std::cout << "订单项目:" << std::endl;
        std::cout << std::left << std::setw(5) << "ID"
                  << std::setw(20) << "名称"
                  << std::setw(10) << "价格"
                  << std::setw(10) << "数量"
                  << std::setw(10) << "小计" << std::endl;
        std::cout << "---------------------------------------------------------------" << std::endl;

        for (const auto &item : order.items)
        {
            std::cout << std::left << std::setw(5) << item.productId
                      << std::setw(20) << item.name
                      << std::setw(10) << item.price
                      << std::setw(10) << item.quantity
                      << std::setw(10) << item.totalPrice << std::endl;
        }
    }

    showFooter();
    pause();
}

void ClientUI::handleOrderCreate()
{
    showHeader("创建订单");

    // 先查看购物车
    auto cartItems = api.getCart();

    if (cartItems.empty())
    {
        std::cout << "购物车为空，无法创建订单。" << std::endl;
    }
    else
    {
        double totalAmount = 0.0;

        std::cout << "购物车商品:" << std::endl;
        std::cout << std::left << std::setw(5) << "ID"
                  << std::setw(20) << "名称"
                  << std::setw(10) << "价格"
                  << std::setw(10) << "数量"
                  << std::setw(10) << "小计" << std::endl;
        std::cout << "---------------------------------------------------------------" << std::endl;

        for (const auto &item : cartItems)
        {
            std::cout << std::left << std::setw(5) << item.productId
                      << std::setw(20) << item.name
                      << std::setw(10) << item.price
                      << std::setw(10) << item.quantity
                      << std::setw(10) << item.totalPrice << std::endl;

            totalAmount += item.totalPrice;
        }

        std::cout << "---------------------------------------------------------------" << std::endl;
        std::cout << "总金额: " << totalAmount << std::endl;

        std::cout << std::endl;
        std::cout << "是否确认创建订单？" << std::endl;
        std::cout << "1. 是" << std::endl;
        std::cout << "2. 否" << std::endl;

        int choice = getMenuChoice(1, 2);

        if (choice == 1)
        {
            int orderId;
            if (api.createOrder(orderId))
            {
                std::cout << "订单创建成功！订单ID: " << orderId << std::endl;
            }
            else
            {
                std::cout << "订单创建失败。" << std::endl;
            }
        }
    }

    showFooter();
    pause();
}

void ClientUI::handleOrderPay()
{
    showHeader("支付订单");

    // 获取所有订单
    auto orders = api.getOrders();

    // 过滤出未支付的订单
    std::vector<OrderInfo> unpaidOrders;
    for (const auto &order : orders)
    {
        if (order.status == "pending" || order.status == "created")
        {
            unpaidOrders.push_back(order);
        }
    }

    if (unpaidOrders.empty())
    {
        std::cout << "没有未支付的订单。" << std::endl;
    }
    else
    {
        std::cout << "未支付的订单:" << std::endl;
        std::cout << std::left << std::setw(5) << "ID"
                  << std::setw(20) << "日期"
                  << std::setw(10) << "状态"
                  << std::setw(10) << "金额" << std::endl;
        std::cout << "---------------------------------------------------------------" << std::endl;

        for (const auto &order : unpaidOrders)
        {
            std::cout << std::left << std::setw(5) << order.id
                      << std::setw(20) << order.date
                      << std::setw(10) << order.status
                      << std::setw(10) << order.totalAmount << std::endl;
        }

        std::cout << "---------------------------------------------------------------" << std::endl;

        std::string orderIdStr = getInput("请输入要支付的订单ID: ");
        try
        {
            int orderId = std::stoi(orderIdStr);

            // 检查订单是否在未支付列表中
            bool found = false;
            for (const auto &order : unpaidOrders)
            {
                if (order.id == orderId)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                std::cout << "该订单不存在或已支付。" << std::endl;
            }
            else
            {
                if (api.payOrder(orderId))
                {
                    std::cout << "订单支付成功！" << std::endl;
                }
                else
                {
                    std::cout << "订单支付失败。" << std::endl;
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cout << "无效的订单ID。" << std::endl;
        }
    }

    showFooter();
    pause();
}

void ClientUI::handleOrderCancel()
{
    showHeader("取消订单");

    // 获取所有订单
    auto orders = api.getOrders();

    // 过滤出可取消的订单（未处理的订单）
    std::vector<OrderInfo> cancelableOrders;
    for (const auto &order : orders)
    {
        if (order.status == "pending" || order.status == "created")
        {
            cancelableOrders.push_back(order);
        }
    }

    if (cancelableOrders.empty())
    {
        std::cout << "没有可取消的订单。" << std::endl;
    }
    else
    {
        std::cout << "可取消的订单:" << std::endl;
        std::cout << std::left << std::setw(5) << "ID"
                  << std::setw(20) << "日期"
                  << std::setw(10) << "状态"
                  << std::setw(10) << "金额" << std::endl;
        std::cout << "---------------------------------------------------------------" << std::endl;

        for (const auto &order : cancelableOrders)
        {
            std::cout << std::left << std::setw(5) << order.id
                      << std::setw(20) << order.date
                      << std::setw(10) << order.status
                      << std::setw(10) << order.totalAmount << std::endl;
        }

        std::cout << "---------------------------------------------------------------" << std::endl;

        std::string orderIdStr = getInput("请输入要取消的订单ID: ");
        try
        {
            int orderId = std::stoi(orderIdStr);

            // 检查订单是否在可取消列表中
            bool found = false;
            for (const auto &order : cancelableOrders)
            {
                if (order.id == orderId)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                std::cout << "该订单不存在或无法取消。" << std::endl;
            }
            else
            {
                if (api.cancelOrder(orderId))
                {
                    std::cout << "订单取消成功！" << std::endl;
                }
                else
                {
                    std::cout << "订单取消失败。" << std::endl;
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cout << "无效的订单ID。" << std::endl;
        }
    }

    showFooter();
    pause();
}
