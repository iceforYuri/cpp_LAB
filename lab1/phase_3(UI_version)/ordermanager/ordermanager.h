#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include "../order/order.h"
#include "../store/store.h"
#include "../user/user.h"
#include <deque>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory> // 为shared_ptr添加

class OrderManager
{
private:
    std::deque<std::shared_ptr<Order>> orderQueue;
    std::string completedOrdersDirectory;
    std::string pendingOrdersFile;

    // 多线程支持
    std::thread processingThread;
    mutable std::mutex queueMutex;
    std::condition_variable queueCondVar;
    std::atomic<bool> stopProcessing;

    // Helper to save a single order to its own file
    bool saveOrderToFile(const Order &order) const;

    // 处理线程的主函数
    void processingLoop(Store &store, std::vector<User *> &allUsers);

    // 内部处理订单的方法
    void processNextOrderInternal(std::shared_ptr<Order> currentOrder, Store &store, std::vector<User *> &allUsers);

public:
    OrderManager(const std::string &ordersDir);
    ~OrderManager();

    // 启动和停止处理线程
    void startProcessingThread(Store &store, std::vector<User *> &allUsers);
    void stopProcessingThread();

    // 提交订单到队列
    std::shared_ptr<Order> submitOrderRequest(const Order &orderRequest);

    // 处理队列中的订单
    void processNextOrder(Store &store, std::vector<User *> &allUsers);
    void processAllPendingOrders(Store &store, std::vector<User *> &allUsers);

    // 添加这个声明！
    size_t getPendingOrderCount() const;

    // 显示待处理订单
    void displayPendingOrders() const;

    // 获取指定用户的所有订单（包括待处理和已完成的）
    void getUserOrders(const std::string &username,
                       std::vector<std::shared_ptr<Order>> &pendingOrders,
                       std::vector<std::shared_ptr<Order>> &completedOrders);
};

#endif // ORDER_MANAGER_H