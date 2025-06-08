#include "ordermanager.h"
#include "../store/store.h"
#include "../order/order.h"
#include "../user/user.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <chrono>
#include <thread>
#include <algorithm>

// 声明外部变量
extern const std::string USER_FILE;
extern bool startProcessing; // 用于控制订单处理线程的启动状态

using namespace std;

// 构造函数，初始化订单目录
OrderManager::OrderManager(const std::string &ordersDir)
    : completedOrdersDirectory(ordersDir), stopProcessing(false)
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

// 析构函数 - 停止处理线程
OrderManager::~OrderManager()
{
    stopProcessingThread();
}

// 启动处理线程
void OrderManager::startProcessingThread(Store &store, std::vector<User *> &allUsers)
{
    // 如果线程已经在运行，先停止
    stopProcessingThread();

    // 重置停止标志
    stopProcessing = false;

    // 启动处理线程
    processingThread = std::thread(&OrderManager::processingLoop, this, std::ref(store), std::ref(allUsers));
    cout << "订单处理线程已启动" << endl;
}

// 停止处理线程
void OrderManager::stopProcessingThread()
{
    if (processingThread.joinable())
    {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stopProcessing = true; // 设置停止标志
        }

        // 通知条件变量，唤醒等待的线程
        queueCondVar.notify_one();

        // 等待线程结束
        processingThread.join();
        cout << "订单处理线程已停止" << endl;
    }
}

// 处理线程的主循环函数
void OrderManager::processingLoop(Store &store, std::vector<User *> &allUsers)
{
    cout << "订单处理线程开始运行..." << endl;

    while (!stopProcessing)
    {
        std::shared_ptr<Order> orderToProcess;
        bool hasOrder = false;

        {
            // 获取队列互斥锁
            std::unique_lock<std::mutex> lock(queueMutex);

            // 等待条件变量：直到队列非空或收到停止信号
            queueCondVar.wait(lock, [this]
                              { return !orderQueue.empty() || stopProcessing; });

            // 如果收到停止信号且队列为空，则退出
            if (stopProcessing && orderQueue.empty())
                break;

            // 如果队列非空，取出队首订单
            if (!orderQueue.empty())
            {
                orderToProcess = orderQueue.front();
                orderQueue.pop_front();
                hasOrder = true;
            }
        }

        // 如果有订单需要处理，则处理它
        if (hasOrder)
        {
            cout << "\n线程正在处理订单 ID: " << orderToProcess->getOrderId()
                 << "，客户: " << orderToProcess->getCustomerUsername() << endl;

            // 处理订单
            processNextOrderInternal(orderToProcess, store, allUsers);
        }
    }

    cout << "订单处理线程已终止" << endl;
}

// 内部处理订单的方法（被处理线程调用）
void OrderManager::processNextOrderInternal(std::shared_ptr<Order> currentOrder, Store &store, std::vector<User *> &allUsers)
{
    cout << "正在处理订单 ID: " << currentOrder->getOrderId()
         << "，客户: " << currentOrder->getCustomerUsername() << endl;
    currentOrder->setStatus("PROCESSING");

    // 找到订单对应的客户
    Customer *customer = dynamic_cast<Customer *>(User::findUser(allUsers, currentOrder->getCustomerUsername()));
    if (!customer)
    {
        cerr << "错误: 找不到订单对应的客户 " << currentOrder->getCustomerUsername() << endl;
        currentOrder->setStatus("FAILED_CUSTOMER_NOT_FOUND");
        saveOrderToFile(*currentOrder);   // 注意这里需要解引用
        currentOrder->setProcessed(true); // 标记处理完成
        return;
    }

    // 第一阶段：重新验证商品（库存可能在队列等待期间发生变化）
    for (const auto &item : currentOrder->getItems())
    {
        Product *product = store.findProductByName(item.productId);
        if (!product)
        {
            cerr << "错误: 商品 \"" << item.productName << "\" 不存在或已下架。订单取消。" << endl;
            currentOrder->setStatus("FAILED_PRODUCT_NOT_FOUND");
            saveOrderToFile(*currentOrder);
            currentOrder->setProcessed(true);
            return;
        }
        if (product->getQuantity() < item.quantity)
        {
            cerr << "错误: 商品 \"" << item.productName << "\" 库存不足 (需要 "
                 << item.quantity << ", 现有 " << product->getQuantity() << ")。订单取消。" << endl;
            currentOrder->setStatus("FAILED_INSUFFICIENT_STOCK");
            saveOrderToFile(*currentOrder);
            currentOrder->setProcessed(true);
            return;
        }
    }

    // 第二阶段：验证客户余额
    if (customer->checkBalance() < currentOrder->getTotalAmount())
    {
        cerr << "错误: 客户 " << customer->getUsername() << " 余额不足。订单取消。" << endl;
        currentOrder->setStatus("FAILED_INSUFFICIENT_FUNDS");
        saveOrderToFile(*currentOrder);
        currentOrder->setProcessed(true);
        return;
    }

    // 第三阶段：处理支付和更新库存
    if (!customer->withdraw(currentOrder->getTotalAmount()))
    {
        cerr << "严重错误: 客户 " << customer->getUsername()
             << " 扣款失败，尽管余额检查已通过。订单: " << currentOrder->getOrderId() << endl;
        currentOrder->setStatus("FAILED_PAYMENT_ERROR");
        saveOrderToFile(*currentOrder);
        currentOrder->setProcessed(true);
        return;
    }

    bool allSellersPaid = true;
    for (const auto &item : currentOrder->getItems())
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
                     << " 失败。订单 ID: " << currentOrder->getOrderId() << endl;
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
                 << "\"。订单 ID: " << currentOrder->getOrderId() << endl;
            allSellersPaid = false;
        }
    }

    // 保存所有更改（用户和商店）
    User::saveUsersToFile(allUsers, USER_FILE);
    store.saveAllProducts();

    // 设置最终订单状态
    if (allSellersPaid)
    {
        currentOrder->setStatus("COMPLETED");
    }
    else
    {
        currentOrder->setStatus("COMPLETED_WITH_PAYMENT_ISSUES");
    }

    cout << "订单 ID: " << currentOrder->getOrderId()
         << " 处理完成。最终状态: " << currentOrder->getStatus() << endl;
    currentOrder->displaySummary();
    saveOrderToFile(*currentOrder);

    // 标记订单已处理完成（关键步骤！）
    currentOrder->setProcessed(true);
}

// 将订单保存到文件
bool OrderManager::saveOrderToFile(const Order &order) const
{
    if (completedOrdersDirectory.empty())
    {
        std::cerr << "错误: 未设置订单目录。" << std::endl;
        return false;
    }

    // 创建基于订单ID的文件名
    std::string filePath = completedOrdersDirectory + "/" + order.getOrderId() + ".txt";
    std::ofstream outFile(filePath);
    if (!outFile)
    {
        std::cerr << "错误: 无法打开文件保存订单: " << filePath << std::endl;
        return false;
    }

    // 保存订单头部信息
    outFile << order.toStringForSaveHeader();

    // 保存订单项
    for (const auto &item : order.getItems())
    {
        outFile << item.toStringForSave() << std::endl;
    }

    outFile.close();
    std::cout << "订单 " << order.getOrderId() << " 已保存到 " << filePath << std::endl;
    return true;
}

// 提交订单到处理队列 (线程安全版本)
std::shared_ptr<Order> OrderManager::submitOrderRequest(const Order &orderRequest)
{
    // 创建订单的共享指针副本
    auto orderPtr = std::make_shared<Order>(orderRequest);

    // 设置订单状态为等待队列处理
    orderPtr->setStatus("PENDING_IN_QUEUE");
    orderPtr->setProcessed(false);

    // 锁定队列
    std::lock_guard<std::mutex> lock(queueMutex);

    // 将订单加入队列
    orderQueue.push_back(orderPtr);

    // 通知处理线程有新订单
    queueCondVar.notify_one();

    cout << "订单 ID: " << orderPtr->getOrderId()
         << " 已提交到处理队列，当前队列长度: " << orderQueue.size() << endl;

    // 返回订单共享指针
    return orderPtr;
}

// 处理队列中的一个订单 (公共方法版，委托给内部方法)
void OrderManager::processNextOrder(Store &store, std::vector<User *> &allUsers)
{
    std::shared_ptr<Order> orderToProcess;
    bool hasOrder = false;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (!orderQueue.empty())
        {
            orderToProcess = orderQueue.front();
            orderQueue.pop_front();
            hasOrder = true;
        }
    }

    if (hasOrder)
    {
        processNextOrderInternal(orderToProcess, store, allUsers);
    }
}

// 处理所有待处理订单 (线程安全版本)
void OrderManager::processAllPendingOrders(Store &store, std::vector<User *> &allUsers)
{
    std::vector<std::shared_ptr<Order>> ordersToProcess;

    // 首先，从队列中安全地取出所有订单
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        while (!orderQueue.empty())
        {
            ordersToProcess.push_back(orderQueue.front());
            orderQueue.pop_front();
        }
    }

    // 如果没有订单，直接返回
    if (ordersToProcess.empty())
    {
        cout << "当前没有待处理订单。" << endl;
        return;
    }

    cout << "\n--- 开始处理 " << ordersToProcess.size() << " 个待处理订单 ---" << endl;

    // 处理获取的订单
    for (auto &order : ordersToProcess)
    {
        processNextOrderInternal(order, store, allUsers);
    }

    cout << "--- 成功处理 " << ordersToProcess.size() << " 个订单 ---" << endl;
}

// 获取待处理订单数量 (线程安全版本)
size_t OrderManager::getPendingOrderCount() const
{
    std::lock_guard<std::mutex> lock(queueMutex);
    return orderQueue.size();
}

// 显示待处理订单 (线程安全版本)
void OrderManager::displayPendingOrders() const
{
    std::lock_guard<std::mutex> lock(queueMutex);

    if (orderQueue.empty())
    {
        cout << "当前队列中没有待处理订单。" << endl;
        return;
    }

    cout << "\n--- 待处理订单队列 ---" << endl;
    int i = 1;
    for (const auto &order : orderQueue)
    {
        cout << i++ << ". 订单 ID: " << order->getOrderId()
             << ", 客户: " << order->getCustomerUsername()
             << ", 金额: ¥" << fixed << setprecision(2) << order->getTotalAmount()
             << ", 状态: " << order->getStatus() << endl;
    }
    cout << "-----------------------------" << endl;
}

// 获取指定用户的所有订单
void OrderManager::getUserOrders(const std::string &username,
                                 std::vector<std::shared_ptr<Order>> &pendingOrders,
                                 std::vector<std::shared_ptr<Order>> &completedOrders)
{
    // 清空传入的向量
    pendingOrders.clear();
    completedOrders.clear();

    // 获取队列中的待处理订单
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        for (const auto &order : orderQueue)
        {
            if (order->getCustomerUsername() == username)
            {
                pendingOrders.push_back(order);
            }
        }
    }

    // 获取已完成的订单
    try
    {
        // 检查订单目录是否存在
        if (!std::filesystem::exists(completedOrdersDirectory))
        {
            std::cerr << "订单目录不存在: " << completedOrdersDirectory << std::endl;
            return;
        }

        // 遍历订单目录中的所有文件
        for (const auto &entry : std::filesystem::directory_iterator(completedOrdersDirectory))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".txt")
            {
                // 从文件名中提取订单ID
                std::string filename = entry.path().filename().string();
                if (filename.find("ORD-") == 0) // 确保是订单文件
                {
                    // 尝试读取订单文件
                    std::ifstream file(entry.path());
                    if (file.is_open())
                    {
                        std::string line;
                        bool isUserOrder = false;
                        std::string orderId, status;
                        double totalAmount = 0.0;
                        time_t timestamp = 0;
                        std::string productId;
                        std::string productName;
                        int quantity;
                        double priceAtPurchase;
                        std::string sellerUsername;

                        // 读取订单头部信息
                        for (int i = 0; i < 6 && std::getline(file, line); i++)
                        {
                            switch (i)
                            {
                            case 0:
                                orderId = line.substr(8); // 识别从传参开始到末尾的字符串
                                break;
                            case 1:
                                if (line.substr(17) == username)
                                {
                                    isUserOrder = true;
                                }
                                else
                                {
                                    isUserOrder = false;
                                    break; // 如果不是用户的订单，则跳出循环
                                }
                                break;
                            case 2:
                                timestamp = std::stoll(line.substr(10));
                                break;

                            case 3:
                                status = line.substr(7);
                                break;

                            case 4:
                                totalAmount = std::stod(line.substr(12));
                                break;
                            case 5:
                                quantity = std::stod(line.substr(11));
                                break;
                            }
                        } // 如果是用户的订单，则创建一个新的订单对象并添加到相应列表
                        std::vector<OrderItem> items;

                        // 读取订单中的所有商品项
                        while (std::getline(file, line))
                        {
                            std::istringstream iss(line);
                            std::string part;
                            std::vector<std::string> parts;

                            // 解析逗号分隔的值
                            while (std::getline(iss, part, ','))
                            {
                                parts.push_back(part);
                            }

                            // 确保解析了足够的部分
                            if (parts.size() >= 5)
                            {
                                productId = parts[0];
                                productName = parts[1];
                                quantity = std::stoi(parts[2]);
                                priceAtPurchase = std::stod(parts[3]);
                                sellerUsername = parts[4];

                                // 创建OrderItem并添加到items向量
                                OrderItem item(productId, productName, quantity, priceAtPurchase, sellerUsername);
                                items.push_back(item);
                            }
                        }

                        if (isUserOrder)
                        {
                            auto order = std::make_shared<Order>(username);
                            order->setStatus(status);

                            // 如果有订单ID，设置它（否则会生成新的ID）
                            if (!orderId.empty())
                            {
                                order->setOrderId(orderId);
                            }

                            if (totalAmount > 0)
                            {
                                // 设置总金额
                                order->setTotalAmount(totalAmount); // 先计算一次
                            }

                            // 如果有时间戳，设置它
                            if (timestamp > 0)
                            {
                                // 我们已经添加了 setTimestamp 方法
                                order->setTimestamp(timestamp);
                            }

                            // 添加所有解析出的订单项到订单中
                            for (const auto &item : items)
                            {
                                order->addItem(item);
                            }

                            // 添加到已完成订单列表
                            // 根据状态判断是已完成还是失败/取消
                            if (status.find("COMPLETED") == 0 ||
                                status.find("FAILED") == 0 ||
                                status.find("CANCELLED") == 0)
                            {
                                completedOrders.push_back(order);
                            }
                        }
                    }
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "读取订单文件时出错: " << e.what() << std::endl;
    }
    // 按时间戳排序（可选）
    // 使用显式的标准库排序算法
    ::std::sort(pendingOrders.begin(), pendingOrders.end(),
                [](const std::shared_ptr<Order> &a, const std::shared_ptr<Order> &b)
                {
                    return a->getTimestamp() > b->getTimestamp(); // 从新到旧
                });

    ::std::sort(completedOrders.begin(), completedOrders.end(),
                [](const std::shared_ptr<Order> &a, const std::shared_ptr<Order> &b)
                {
                    return a->getTimestamp() > b->getTimestamp(); // 从新到旧
                });
}