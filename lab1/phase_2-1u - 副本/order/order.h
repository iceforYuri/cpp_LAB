#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <vector>
#include <ctime>
#include <iomanip>  // For std::setprecision
#include <iostream> // For display methods

// Forward declarations
class Product; // From store.h
class User;    // From user.h
class Store;   // From store.h

class OrderItem
{

public:
    std::string productId;
    std::string productName;
    int quantity;
    double priceAtPurchase;
    std::string sellerUsername;

    OrderItem(std::string pid, std::string pName, int qty, double price, std::string sUsername)
        : productId(std::move(pid)), productName(std::move(pName)), quantity(qty),
          priceAtPurchase(price), sellerUsername(std::move(sUsername)) {}

    void display() const;

    std::string toStringForSave() const;
};

class Order
{
private:
    std::string orderId;
    std::string customerUsername;
    std::vector<OrderItem> items;
    double totalAmount;
    bool isProcessed; // Indicates if the order has been processed
    time_t orderTimestamp;
    std::string status; // e.g., "PENDING_CONFIRMATION", "COMPLETED", "CANCELLED_INSUFFICIENT_STOCK", "CANCELLED_INSUFFICIENT_FUNDS", "CANCELLED_BY_USER"
    static std::string generateOrderId();

public:
    Order(std::string custUsername);
    // 新增默认构造函数
    Order() : customerUsername(""),
              totalAmount(0.0),
              orderTimestamp(std::time(nullptr)), status("UNINITIALIZED"),
              isProcessed(false)
    {
        orderId = "EMPTY-" + std::to_string(orderTimestamp);
    }
    bool addItemFromProduct(const Product &product, int quantity); // Adds item and updates total
    void addItem(const OrderItem &item);                           // Adds an existing OrderItem and updates total
    void calculateTotalAmount();                                   // Recalculates total based on items

    // Getters
    std::string getOrderId() const { return orderId; }
    std::string getCustomerUsername() const { return customerUsername; }
    const std::vector<OrderItem> &getItems() const { return items; }
    double getTotalAmount() const { return totalAmount; }
    std::string getStatus() const { return status; }
    time_t getTimestamp() const { return orderTimestamp; } // Setters
    void setStatus(const std::string &newStatus) { status = newStatus; }
    void setProcessed(bool processed) { isProcessed = processed; }
    bool getProcessed() const { return isProcessed; }

    // 添加必要的 setter 方法
    void setTimestamp(time_t timestamp) { orderTimestamp = timestamp; }
    void setOrderId(const std::string &id) { orderId = id; }
    void setTotalAmount(double amount) { totalAmount = amount; }

    void displaySummary() const;

    std::string toStringForSaveHeader() const;
    // Persistence (Optional - for saving orders to files)
    // bool saveToFile(const std::string& directoryPath) const;
    // static Order loadFromFile(const std::string& filePath);
};

#endif // ORDER_H