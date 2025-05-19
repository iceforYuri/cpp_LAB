#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include "../order/order.h"
#include "../store/store.h"
#include "../user/user.h"
#include <deque>
#include <string>
#include <vector>

class OrderManager
{
private:
    std::deque<Order> orderQueue;
    std::string completedOrdersDirectory; // Directory to save completed/failed orders
    std::string pendingOrdersFile;        // File to persist the queue (optional for now)

    // Helper to save a single order to its own file
    bool saveOrderToFile(const Order &order) const;
    // Helper to load pending orders from file (optional for now)
    // void loadPendingOrders();

public:
    OrderManager(const std::string &ordersDir); // Constructor takes directory for storing orders

    // Adds an order request (created from cart) to the queue
    Order &submitOrderRequest(Order &orderRequest);

    // Processes one order from the front of the queue
    // Needs access to the main user list and store object
    void processNextOrder(Store &store, std::vector<User *> &allUsers);

    // Processes all orders currently in the queue
    void processAllPendingOrders(Store &store, std::vector<User *> &allUsers);

    // Gets the current size of the order queue
    size_t getPendingOrderCount() const;

    // (Optional) Display pending orders
    void displayPendingOrders() const;

    // (Optional) Persist the current order queue to a file
    // bool persistQueue() const;
};

#endif // ORDER_MANAGER_H