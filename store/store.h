#ifndef STORE_H
#define STORE_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>

// 前向声明 User
class User;

// --- Product 基类和派生类 (保持不变) ---
class Product
{
protected:
    std::string name;
    std::string description;
    double originalPrice;
    int quantity;
    double discountRate;

public:
    Product(std::string name, std::string desc, double price, int qty)
        : name(name), description(desc), originalPrice(price), quantity(qty), discountRate(0.0) {}
    virtual ~Product() = default;
    virtual double getPrice() const { return originalPrice * (1.0 - discountRate); }
    virtual void display() const; // 实现移到 cpp
    virtual std::string getType() const = 0;
    virtual void save(std::ofstream &ofs) const; // 实现移到 cpp

    // Getters and Setters (保持不变)
    std::string getName() const { return name; }
    std::string getDescription() const { return description; }
    double getOriginalPrice() const { return originalPrice; }
    int getQuantity() const { return quantity; }
    double getDiscountRate() const { return discountRate; }
    void setName(const std::string &newName) { name = newName; }
    void setDescription(const std::string &newDesc) { description = newDesc; }
    void setOriginalPrice(double newPrice)
    {
        if (newPrice >= 0)
            originalPrice = newPrice;
    }
    void setQuantity(int newQuantity)
    {
        if (newQuantity >= 0)
            quantity = newQuantity;
    }
    void setDiscountRate(double newRate)
    {
        if (newRate >= 0.0 && newRate <= 1.0)
            discountRate = newRate;
    }

    // 静态辅助函数移到 Page 类或保持独立
    // static void clearInputBufferStore();
};

class Book : public Product
{ /* ... 保持不变 ... */
private:
    std::string author;
    std::string isbn;

public:
    Book(std::string name, std::string desc, double price, int qty, std::string auth, std::string isbn_num)
        : Product(name, desc, price, qty), author(auth), isbn(isbn_num) {}
    std::string getType() const override { return "Book"; }
    void display() const override;                // 实现移到 cpp
    void save(std::ofstream &ofs) const override; // 实现移到 cpp
};
class Clothing : public Product
{ /* ... 保持不变 ... */
private:
    std::string size;
    std::string color;

public:
    Clothing(std::string name, std::string desc, double price, int qty, std::string sz, std::string clr)
        : Product(name, desc, price, qty), size(sz), color(clr) {}
    std::string getType() const override { return "Clothing"; }
    void display() const override;                // 实现移到 cpp
    void save(std::ofstream &ofs) const override; // 实现移到 cpp
};
class Food : public Product
{ /* ... 保持不变 ... */
private:
    std::string expirationDate;

public:
    Food(std::string name, std::string desc, double price, int qty, std::string expDate)
        : Product(name, desc, price, qty), expirationDate(expDate) {}
    std::string getType() const override { return "Food"; }
    void display() const override;                // 实现移到 cpp
    void save(std::ofstream &ofs) const override; // 实现移到 cpp
};

// --- Store Class (只包含数据和核心逻辑) ---
class Store
{
private:
    std::vector<Product *> products;
    std::string storeFilename;

    // Helper to find product by name (保持 private)
    Product *findProductByName(const std::string &name);

public:
    // Constructor & Destructor
    Store(const std::string &filename);
    ~Store();

    // File Operations
    bool loadProductsFromFile();
    bool saveProductsToFile();

    // --- Core Store Functionality (供 Page 子类调用) ---
    // 显示所有商品 (const)
    void displayAllProducts() const;
    // 搜索商品 (const) - 返回找到的商品列表或直接显示
    std::vector<Product *> searchProductsByName(const std::string &searchTerm) const;
    // 添加商品 (需要 User* 判断权限) - 返回是否成功
    bool addProduct(User *currentUser, const Product &newProductData); // 传入具体商品数据
    // 管理商品 (需要 User* 判断权限) - 返回是否成功
    bool manageProductPrice(User *currentUser, const std::string &productName, double newPrice);
    bool manageProductQuantity(User *currentUser, const std::string &productName, int newQuantity);
    bool manageProductDiscount(User *currentUser, const std::string &productName, double newDiscount);
    // 应用分类折扣 (需要 User* 判断权限) - 返回是否成功
    bool applyCategoryDiscount(User *currentUser, const std::string &category, double discount);

    // 获取商品列表的只读访问权限 (如果需要)
    const std::vector<Product *> &getProducts() const { return products; }

    // 移除 enterStore, display*Menu, handle*Action
};

#endif // STORE_H