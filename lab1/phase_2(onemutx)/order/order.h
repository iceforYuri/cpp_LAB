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
    time_t orderTimestamp;
    std::string status; // e.g., "PENDING_CONFIRMATION", "COMPLETED", "CANCELLED_INSUFFICIENT_STOCK", "CANCELLED_INSUFFICIENT_FUNDS", "CANCELLED_BY_USER"

    bool isProcessed;
    static std::string generateOrderId();

public:
    Order(std::string custUsername);

    bool addItemFromProduct(const Product &product, int quantity); // Adds item and updates total
    void calculateTotalAmount();                                   // Recalculates total based on items

    // Getters
    std::string getOrderId() const { return orderId; }
    std::string getCustomerUsername() const { return customerUsername; }
    const std::vector<OrderItem> &getItems() const { return items; }
    double getTotalAmount() const { return totalAmount; }
    std::string getStatus() const { return status; }
    time_t getTimestamp() const { return orderTimestamp; }

    // Setters
    void setStatus(const std::string &newStatus) { status = newStatus; }
    void setProcessed(bool processed) { isProcessed = processed; }
    bool getProcessed() const { return isProcessed; }

    void displaySummary() const;

    std::string toStringForSaveHeader() const;
    // Persistence (Optional - for saving orders to files)
    // bool saveToFile(const std::string& directoryPath) const;
    // static Order loadFromFile(const std::string& filePath);
};

#endif // ORDER_H