#ifndef USER_H
#define USER_H

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <limits>

class User
{
protected: // protected成员可被派生类访问
    std::string username;
    std::string password;
    std::string userType; // 用户类型
    double balance;
    static void clearInputBuffer() // 清空输入缓冲区
    {
        std::cin.clear();                                                   // 清除错误标志
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 忽略剩余输入
    }

public:
    // 构造函数
    User(std::string username, std::string password, double balance = 0.0,
         bool customer = false, bool seller = false, bool admin = false);
    User();
    virtual ~User() = default; // 虚析构函数

    // 纯虚函数，使User成为抽象类
    virtual std::string getUserType() const = 0;

    // 账户操作函数
    void displayInfo(int index = -1) const;
    bool changePassword(const std::string &oldPassword, const std::string &newPassword);
    bool deposit(double amount);
    bool withdraw(double amount);
    double checkBalance() const;

    //
    static User *registerUser(std::vector<User *> &users);    // 注册新用户
    static User *userLogin(const std::vector<User *> &users); // 用户登录 (获取输入并验证)

    // 文件操作函数
    static bool saveUsersToFile(const std::vector<User *> &users, const std::string &filename);
    static std::vector<User *> loadUsersFromFile(const std::string &filename);
    static User *findUser(const std::vector<User *> &users, const std::string &username);
    static void displayAllUsers(const std::vector<User *> &users);
    static bool isUsernameExists(const std::vector<User *> &users, const std::string &username);

    // Getter 函数
    std::string getUsername() const;
    std::string getPassword() const;

    // Setter 函数
    void setUsername(const std::string &newUsername);
    void setPassword(const std::string &newPassword);
    void setBalance(double newBalance);

    // 用户验证
    static User *login(const std::vector<User *> &users, const std::string &username, const std::string &password);
};

// 消费者类
class Customer : public User
{
public:
    Customer(std::string username, std::string password, double balance = 0.0);
    Customer();
    std::string getUserType() const override { return "customer"; }
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