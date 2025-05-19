#ifndef CART_H
#define CART_H

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <limits>
#include <iomanip> // For output formatting

// Forward declarations to avoid circular dependencies
class Product;
class Store;
class User;     // For user interactions during checkout
class Customer; // Specifically for the buyer

// 购物车中的商品项
struct CartItem
{
    std::string productId; // 商品名称或唯一ID
    std::string productName;
    int quantity;
    double priceAtAddition; // 添加到购物车时的价格
    std::string sellerUsername;

    CartItem(std::string pid = "", std::string pName = "", int qty = 0, double price = 0.0, std::string sName = "")
        : productId(pid), productName(pName), quantity(qty), priceAtAddition(price), sellerUsername(sName) {}
};

class Cart
{
private:
    std::string ownerUsername;
    std::string cartDirectoryPath; // e.g., "./cart/carts"
    std::vector<CartItem> items;

    std::string getCartFilePath() const;
    void loadFromFile();
    bool saveToFile() const;

public:
    Cart(const std::string &username, const std::string &cartDir); // Constructor
    ~Cart();                                                       // Destructor, might save cart on destruction if needed

    bool addItem(const Product &product, int quantity);
    bool removeItem(const std::string &productName);
    bool updateItemQuantity(const std::string &productName, int newQuantity, Store &store); // Needs store to check stock
    void displayCart(Store &store) const;                                                   // Needs store to get current product info

    // Checkout needs access to buyer's balance, store for stock, and all users for seller payment
    bool checkout(Customer *buyer, Store &store, std::vector<User *> &allUsers);
    bool isEmpty() const;
    void clearCartAndFile(); // Clears items in memory and deletes the cart file
};

#endif // CART_H