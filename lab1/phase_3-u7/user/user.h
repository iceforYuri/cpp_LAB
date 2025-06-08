#ifndef USER_H
#define USER_H

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <limits>
#include <iomanip>          // For output formatting in cart display
#include "../order/order.h" // For OrderItem

// 前向声明
class Product;
class Store;

// 购物车中的商品项 (现在作为 Customer 类的内部结构或辅助结构)
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

class User
{
protected:
    std::string username;
    std::string password;
    std::string userType;
    double balance;
    static void clearInputBuffer()
    {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

public:
    User(std::string username, std::string password, double balance = 0.0,
         bool is_customer = false, bool is_seller = false, bool is_admin = false);
    User();
    virtual ~User() = default;

    virtual std::string getUserType() const = 0;
    void displayInfo(int index = -1) const;
    bool changePassword(const std::string &oldPassword, const std::string &newPassword);
    bool deposit(double amount);
    bool withdraw(double amount);
    double checkBalance() const;

    static User *registerUser(std::vector<User *> &users);
    static User *userLogin(const std::vector<User *> &users);
    static bool saveUsersToFile(const std::vector<User *> &users, const std::string &filename);
    static std::vector<User *> loadUsersFromFile(const std::string &filename);
    static User *findUser(const std::vector<User *> &users, const std::string &username);
    static void displayAllUsers(const std::vector<User *> &users);
    static bool isUsernameExists(const std::vector<User *> &users, const std::string &username);

    std::string getUsername() const;
    std::string getPassword() const;
    void setUsername(const std::string &newUsername);
    void setPassword(const std::string &newPassword);
    void setBalance(double newBalance);

    static User *login(const std::vector<User *> &users, const std::string &username, const std::string &password);
};

// 消费者类
class Customer : public User
{
private:
    std::string cartDirectoryPath; // 存储购物车文件的目录路径

    // 购物车文件操作的辅助方法
    std::string getCartFilePath() const;

public:
    Customer(std::string username, std::string password, const std::string &cartDir, double balance = 0.0);
    Customer(const std::string &cartDir); // 默认构造函数
    std::string getUserType() const override { return "customer"; }
    std::vector<CartItem> shoppingCartItems; // 直接在 Customer 中存储购物车项

    void loadCartFromFile();
    bool saveCartToFile() const;

    // 购物车管理方法
    bool addToCart(const Product &product, int quantity);
    bool removeCartItem(const std::string &productName);
    bool updateCartItemQuantity(const std::string &productName, int newQuantity, Store &store); // 需要 store 检查库存
    void viewCart(Store &store, std::vector<User *> &allUsers);                                 // 查看并管理购物车，结算也在这里处理
    bool isCartEmpty() const;
    void clearCartAndFile(); // 结算后清空购物车及文件
};

// 商家类
class Seller : public User
{
public:
    Seller(std::string username, std::string password, double balance = 0.0);
    Seller();
    std::string getUserType() const override { return "seller"; }
};

// 管理员类
class Admin : public User
{
public:
    Admin(std::string username, std::string password, double balance = 0.0);
    Admin();
    std::string getUserType() const override { return "admin"; }
};

#endif // USER_H