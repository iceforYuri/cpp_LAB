#include "order.h"
#include "../store/store.h"
#include "../user/user.h" // For Product and User definitions
#include <sstream>
#include <chrono>    // For generating unique ID
#include <fstream>   // For file operations (if saving orders)
#include <algorithm> // For std::remove_if (if needed for item management)

// --- OrderItem Implementation ---
void OrderItem::display() const
{
    std::cout << "    - 商品名称: " << productName << " (ID: " << productId << ")" << std::endl;
    std::cout << "      购买数量: " << quantity << ", 商品单价: ¥" << priceAtPurchase
              << ", 商品总价: ¥" << (quantity * priceAtPurchase)
              << ", 商家: " << sellerUsername << std::endl;
}

// --- Order Implementation ---
std::string Order::generateOrderId()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now); // Convert to time_t for tm structure
    std::tm now_tm = *std::localtime(&now_c);               // Get local time structure

    // Get milliseconds part
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << "ORD-";
    ss << std::put_time(&now_tm, "%Y%m%d-%H%M%S");                // Format: YYYYMMDD-HHMMSS
    ss << "-" << std::setfill('0') << std::setw(3) << ms.count(); // Append milliseconds, zero-padded

    return ss.str();
}

Order::Order(std::string custUsername)
    : customerUsername(std::move(custUsername)), totalAmount(0.0), status("PENDING_CREATION")
{
    orderId = generateOrderId();
    orderTimestamp = std::time(nullptr);
}

bool Order::addItemFromProduct(const Product &product, int quantity)
{
    if (quantity <= 0)
        return false;
    // Note: Stock check should happen before calling this, or this method could also check
    // For now, assumes stock check is done externally before adding to order object
    items.emplace_back(product.getName(), product.getName(), quantity, product.getPrice(), product.getSellerUsername());
    calculateTotalAmount(); // Recalculate total every time an item is added
    return true;
}

void Order::calculateTotalAmount()
{
    totalAmount = 0;
    for (const auto &item : items)
    {
        totalAmount += item.priceAtPurchase * item.quantity;
    }
}

void Order::displaySummary() const
{
    std::cout << "\n--- 订单汇总 ---" << std::endl;
    std::cout << "订单 ID: " << orderId << std::endl;
    std::cout << "买家: " << customerUsername << std::endl;
    std::cout << "日期: " << std::asctime(std::localtime(&orderTimestamp)); // ctime adds newline
    std::cout << "状态: " << status << std::endl;
    std::cout << "下单物品:" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    for (const auto &item : items)
    {
        item.display();
    }
    std::cout << "---------------------" << std::endl;
    std::cout << "总花费: ¥" << totalAmount << std::endl;
    std::cout << "---------------------" << std::endl;
    std::cout << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
}
// 转为CSV格式字符串
std::string OrderItem::toStringForSave() const
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    // Simple CSV-like, ensure no commas in your string fields or use a different delimiter/quoting
    ss << productId << "," << productName << "," << quantity << "," << priceAtPurchase << "," << sellerUsername;
    return ss.str();
}
std::string Order::toStringForSaveHeader() const
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "OrderID:" << orderId << std::endl;
    ss << "CustomerUsername:" << customerUsername << std::endl;
    ss << "Timestamp:" << orderTimestamp << std::endl; // Save as raw time_t
    ss << "Status:" << status << std::endl;
    ss << "TotalAmount:" << totalAmount << std::endl;
    ss << "ItemsCount:" << items.size() << std::endl; // Useful for loading
    return ss.str();
}

// Optional: Implement saveToFile and loadFromFile if you want to persist orders
// bool Order::saveToFile(const std::string& directoryPath) const { ... }
// Order Order::loadFromFile(const std::string& filePath) { ... }