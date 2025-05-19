#include "ordermanager.h"
#include "../store/store.h"
#include "../user/user.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <chrono>
#include <thread>

// 声明外部变量
extern const std::string USER_FILE;

using namespace std;

// 构造函数，初始化订单目录
OrderManager::OrderManager(const std::string &ordersDir)
    : completedOrdersDirectory(ordersDir)
{
    // 确保订单目录存在
    if (!std::filesystem::exists(completedOrdersDirectory))
    {
        try
        {
            std::filesystem::create_directories(completedOrdersDirectory);
            cout << "已创建订单目录：" << completedOrdersDirectory << endl;
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            cerr << "创建订单目录失败: " << e.what() << endl;
        }
    }

    // 设置待处理订单文件路径
    pendingOrdersFile = completedOrdersDirectory + "/pending_orders.txt";
}

// 提交订单到处理队列
Order &OrderManager::submitOrderRequest(Order &orderRequest)
{
    // 设置订单状态为等待队列处理
    orderRequest.setStatus("PENDING_IN_QUEUE");
    orderRequest.setProcessed(false); // 标记订单为未处理

    // 将订单加入队列
    orderQueue.push_back(orderRequest);

    cout << "订单 ID: " << orderRequest.getOrderId()
         << " 已提交到处理队列，当前队列长度: " << orderQueue.size() << endl;

    return orderQueue.back(); // 返回引用，方便后续操作
}

// 处理队列中的一个订单
void OrderManager::processNextOrder(Store &store, std::vector<User *> &allUsers)
{
    if (orderQueue.empty())
    {
        return;
    }

    // 取出队首订单
    Order currentOrder = orderQueue.front();
    orderQueue.pop_front();

    cout << "\n正在处理订单 ID: " << currentOrder.getOrderId()
         << "，客户: " << currentOrder.getCustomerUsername() << endl;
    currentOrder.setStatus("PROCESSING");

    // 找到订单对应的客户
    Customer *customer = dynamic_cast<Customer *>(User::findUser(allUsers, currentOrder.getCustomerUsername()));
    if (!customer)
    {
        cerr << "错误: 找不到订单对应的客户 " << currentOrder.getCustomerUsername() << endl;
        currentOrder.setStatus("FAILED_CUSTOMER_NOT_FOUND");
        saveOrderToFile(currentOrder);
        return;
    }

    // 第一阶段：重新验证商品（库存可能在队列等待期间发生变化）
    for (const auto &item : currentOrder.getItems())
    {
        Product *product = store.findProductByName(item.productId);
        if (!product)
        {
            cerr << "错误: 商品 \"" << item.productName << "\" 不存在或已下架。订单取消。" << endl;
            currentOrder.setStatus("FAILED_PRODUCT_NOT_FOUND");
            saveOrderToFile(currentOrder);
            return;
        }
        if (product->getQuantity() < item.quantity)
        {
            cerr << "错误: 商品 \"" << item.productName << "\" 库存不足 (需要 "
                 << item.quantity << ", 现有 " << product->getQuantity() << ")。订单取消。" << endl;
            currentOrder.setStatus("FAILED_INSUFFICIENT_STOCK");
            saveOrderToFile(currentOrder);
            return;
        }
    }

    // 第二阶段：验证客户余额
    if (customer->checkBalance() < currentOrder.getTotalAmount())
    {
        cerr << "错误: 客户 " << customer->getUsername() << " 余额不足。订单取消。" << endl;
        currentOrder.setStatus("FAILED_INSUFFICIENT_FUNDS");
        saveOrderToFile(currentOrder);
        return;
    }

    // 第三阶段：处理支付和更新库存（事务块）
    if (!customer->withdraw(currentOrder.getTotalAmount()))
    {
        cerr << "严重错误: 客户 " << customer->getUsername()
             << " 扣款失败，尽管余额检查已通过。订单: " << currentOrder.getOrderId() << endl;
        currentOrder.setStatus("FAILED_PAYMENT_ERROR");
        saveOrderToFile(currentOrder);
        return;
    }

    bool allSellersPaid = true;
    for (const auto &item : currentOrder.getItems())
    {
        Product *product = store.findProductByName(item.productId);
        // 更新商品库存
        product->setQuantity(product->getQuantity() - item.quantity);

        // 向卖家转账
        User *seller = User::findUser(allUsers, item.sellerUsername);
        if (seller)
        {
            double itemAmount = item.priceAtPurchase * item.quantity;
            if (!seller->deposit(itemAmount))
            {
                cerr << "警告: 向商家 " << item.sellerUsername
                     << " 支付商品 " << item.productName
                     << " 的款项 ¥" << fixed << setprecision(2) << itemAmount
                     << " 失败。订单 ID: " << currentOrder.getOrderId() << endl;
                allSellersPaid = false;
            }
            else
            {
                cout << "已向商家 " << item.sellerUsername
                     << " 支付 ¥" << fixed << setprecision(2) << itemAmount << endl;
            }
        }
        else if (!item.sellerUsername.empty())
        {
            cerr << "警告: 找不到商品 \"" << item.productName
                 << "\" 的商家 \"" << item.sellerUsername
                 << "\"。订单 ID: " << currentOrder.getOrderId() << endl;
            allSellersPaid = false;
        }
    }

    // 保存所有更改（用户和商店）
    User::saveUsersToFile(allUsers, USER_FILE);
    store.saveAllProducts();

    // 设置最终订单状态
    if (allSellersPaid)
    {
        currentOrder.setStatus("COMPLETED");
    }
    else
    {
        currentOrder.setStatus("COMPLETED_WITH_PAYMENT_ISSUES");
    }

    cout << "订单 ID: " << currentOrder.getOrderId()
         << " 处理完成。最终状态: " << currentOrder.getStatus() << endl;

    currentOrder.setProcessed(true); // 标记订单为已处理
    currentOrder.displaySummary();
    saveOrderToFile(currentOrder);
}

// 处理所有待处理订单
void OrderManager::processAllPendingOrders(Store &store, std::vector<User *> &allUsers)
{
    size_t orderCount = orderQueue.size();
    if (orderCount == 0)
    {
        cout << "当前没有待处理订单。" << endl;
        return;
    }

    cout << "\n--- 开始处理 " << orderCount << " 个待处理订单 ---" << endl;
    int processedCount = 0;

    while (!orderQueue.empty())
    {
        processNextOrder(store, allUsers);
        processedCount++;
    }

    cout << "--- 成功处理 " << processedCount << " 个订单 ---" << endl;
}

// 获取待处理订单数量
size_t OrderManager::getPendingOrderCount() const
{
    return orderQueue.size();
}

// 显示待处理订单
void OrderManager::displayPendingOrders() const
{
    if (orderQueue.empty())
    {
        cout << "当前队列中没有待处理订单。" << endl;
        return;
    }

    cout << "\n--- 待处理订单队列 ---" << endl;
    int i = 1;
    for (const auto &order : orderQueue)
    {
        cout << i++ << ". 订单 ID: " << order.getOrderId()
             << ", 客户: " << order.getCustomerUsername()
             << ", 金额: ¥" << fixed << setprecision(2) << order.getTotalAmount()
             << ", 状态: " << order.getStatus() << endl;
    }
    cout << "-----------------------------" << endl;
}

// 将订单保存到文件
bool OrderManager::saveOrderToFile(const Order &order) const
{
    if (completedOrdersDirectory.empty())
    {
        cerr << "错误: 未设置订单目录。" << endl;
        return false;
    }

    // 创建基于订单ID的文件名
    std::string filePath = completedOrdersDirectory + "/" + order.getOrderId() + ".txt";
    std::ofstream outFile(filePath);
    if (!outFile)
    {
        cerr << "错误: 无法打开文件保存订单: " << filePath << endl;
        return false;
    }

    // 保存订单头部信息
    outFile << order.toStringForSaveHeader();

    // 保存订单项
    for (const auto &item : order.getItems())
    {
        outFile << item.toStringForSave() << endl;
    }

    outFile.close();
    cout << "订单 " << order.getOrderId() << " 已保存到 " << filePath << endl;
    return true;
}