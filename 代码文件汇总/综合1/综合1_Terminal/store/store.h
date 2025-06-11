#ifndef STORE_H
#define STORE_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <map>
#include <filesystem>

// 前向声明 User
class User;

// --- Product 基类和派生类 ---
class Product
{
protected:
    std::string name;
    std::string description;
    double originalPrice;
    int quantity;
    double discountRate;
    std::string sellerUsername; // 添加商品所属商家

public:
    Product(std::string name, std::string desc, double price, int qty, std::string seller = "")
        : name(name), description(desc), originalPrice(price), quantity(qty),
          discountRate(0.0), sellerUsername(seller) {}
    virtual ~Product() = default;
    virtual double getPrice() const { return originalPrice * (1.0 - discountRate); }
    virtual void display() const;
    virtual std::string getType() const = 0;
    virtual void save(std::ofstream &ofs) const;

    // Getters and Setters
    std::string getName() const { return name; }
    std::string getDescription() const { return description; }
    double getOriginalPrice() const { return originalPrice; }
    int getQuantity() const { return quantity; }
    double getDiscountRate() const { return discountRate; }
    std::string getSellerUsername() const { return sellerUsername; } // 获取商品所属商家

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
    void setSellerUsername(const std::string &seller) { sellerUsername = seller; } // 设置商品所属商家
};

class Book : public Product
{
private:
    std::string author;
    std::string isbn;

public:
    Book(std::string name, std::string desc, double price, int qty,
         std::string auth, std::string isbn_num, std::string seller = "")
        : Product(name, desc, price, qty, seller), author(auth), isbn(isbn_num) {}
    std::string getType() const override { return "Book"; }
    void display() const override;
    void save(std::ofstream &ofs) const override;

    // 新增的 getters
    std::string getAuthor() const { return author; }
    std::string getIsbn() const { return isbn; }
};

class Clothing : public Product
{
private:
    std::string size;
    std::string color;

public:
    Clothing(std::string name, std::string desc, double price, int qty,
             std::string sz, std::string clr, std::string seller = "")
        : Product(name, desc, price, qty, seller), size(sz), color(clr) {}
    std::string getType() const override { return "Clothing"; }
    void display() const override;
    void save(std::ofstream &ofs) const override;

    // 新增的 getters
    std::string getSize() const { return size; }
    std::string getColor() const { return color; }
};

class Food : public Product
{
private:
    std::string expirationDate;

public:
    Food(std::string name, std::string desc, double price, int qty,
         std::string expDate, std::string seller = "")
        : Product(name, desc, price, qty, seller), expirationDate(expDate) {}
    std::string getType() const override { return "Food"; }
    void display() const override;
    void save(std::ofstream &ofs) const override;

    // 新增的 getters
    std::string getExpirationDate() const { return expirationDate; }
};

// --- Store Class ---
class Store
{
private:
    std::vector<Product *> allProducts;                           // 存储所有商家的商品
    std::map<std::string, std::vector<Product *>> sellerProducts; // 每个商家的商品映射
    std::string storeDirectory;                                   // 商品文件所在目录

    // 辅助方法
    std::string getSellerFilename(const std::string &username) const;

    bool saveProductsForSeller(const std::string &sellerUsername);
    bool ensureDirectoryExists(const std::string &path) const;

public:
    // 构造函数接收目录名
    Store(const std::string &directory);
    ~Store();

    Product *findProductByName(const std::string &name, const std::string &sellerUsername = "");

    // 文件操作
    bool loadAllProducts();
    bool loadSellerProducts(const std::string &sellerUsername);
    bool saveAllProducts();

    // 显示功能
    void displayAllProducts() const;                                     // 显示所有商品
    void displaySellerProducts(const std::string &sellerUsername) const; // 显示指定商家的商品

    // 搜索功能
    std::vector<Product *> searchProductsByName(const std::string &searchTerm, const std::string &sellerUsername = "") const;

    // 商家商品管理功能
    bool createBook(User *currentUser, const std::string &name, const std::string &desc,
                    double price, int qty, const std::string &author, const std::string &isbn);
    bool createClothing(User *currentUser, const std::string &name, const std::string &desc,
                        double price, int qty, const std::string &size, const std::string &color);
    bool createFood(User *currentUser, const std::string &name, const std::string &desc,
                    double price, int qty, const std::string &expDate);

    bool manageProductPrice(User *currentUser, const std::string &productName, double newPrice);
    bool manageProductQuantity(User *currentUser, const std::string &productName, int newQuantity);
    bool manageProductDiscount(User *currentUser, const std::string &productName, double newDiscount);
    bool applyCategoryDiscount(User *currentUser, const std::string &category, double discount);

    // 获取商品
    const std::vector<Product *> &getProducts() const { return allProducts; }
    std::vector<Product *> getSellerProducts(const std::string &sellerUsername) const;
};

#endif // STORE_H