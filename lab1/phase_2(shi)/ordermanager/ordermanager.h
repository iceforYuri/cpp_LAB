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

class OrderManager
{
private:
    std::deque<Order> orderQueue;
    std::string completedOrdersDirectory;
    std::string pendingOrdersFile;

    // 多线程支持
    std::thread processingThread;
    mutable std::mutex queueMutex;
    std::condition_variable queueCondVar;
    std::atomic<bool> stopProcessing;

    // Helper to save a single order to its own file
    bool saveOrderToFile(const Order &order) const;

    // 新增：处理线程的主函数
    void processingLoop(Store &store, std::vector<User *> &allUsers);

    // 新增：内部处理订单的方法（被处理线程调用）
    void processNextOrderInternal(Order &orderToProcess, Store &store, std::vector<User *> &allUsers);

public:
    OrderManager(const std::string &ordersDir);
    ~OrderManager(); // 析构函数处理线程清理

    // 启动和停止处理线程
    void startProcessingThread(Store &store, std::vector<User *> &allUsers);
    void stopProcessingThread();

    // Adds an order request (created from cart) to the queue
    Order &submitOrderRequest(Order &orderRequest);

    // Processes one order from the front of the queue (仍然保留，但主要由处理线程调用)
    void processNextOrder(Store &store, std::vector<User *> &allUsers);

    // Processes all orders currently in the queue
    void processAllPendingOrders(Store &store, std::vector<User *> &allUsers);

    // Gets the current size of the order queue (线程安全)
    size_t getPendingOrderCount() const;

    // 显示待处理订单
    void displayPendingOrders() const;
};

#endif // ORDER_MANAGER_H