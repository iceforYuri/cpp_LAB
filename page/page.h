#ifndef PAGE_H
#define PAGE_H

#include "../store/store.h" // 包含 Store 类定义
#include <vector>
#include <string> // 需要包含 string

// 前向声明 User 类，因为 Page 方法使用了 User*
// 避免在头文件中包含 user.h，减少编译依赖
class User;

// --- Page 类定义 ---
// 用于管理不同状态下的菜单显示
class Page
{
protected:
    // 私有静态辅助方法：清除输入缓冲区 (Page 类内部使用)
    // 定义移到 page.cpp
    static void clearInputBuffer();

public:
    // --- 成员方法：菜单显示 ---
    // 主菜单 (未登录)
    int pagemain(std::vector<User *> &users, User *&currentUser);
    // 消费者菜单 (登录后)
    int pagemainCustomer(User *currentUser, std::vector<User *> &users);
    // 商家菜单 (登录后)
    int pagemainSeller(User *currentUser, std::vector<User *> &users);
    // 管理员菜单 (如果需要可以添加)
    int pagemainAdmin(User *currentUser, std::vector<User *> &users);

    virtual int pagestore(User *currentUser, std::vector<User *> &users, Store &store);
};

// --- Page 派生类 ---
class Customer_page : public Page
{
public:
    // 实现商城页面 (确保有 Store& store 参数)
    int pagestore(User *currentUser, std::vector<User *> &users, Store &store) override;
    // 可以覆盖基类的账户菜单方法，如果需要特定行为
};

class Seller_page : public Page
{
public:
    // 实现商城页面 (确保有 Store& store 参数)
    int pagestore(User *currentUser, std::vector<User *> &users, Store &store) override;
    // 可以覆盖基类的账户菜单方法，如果需要特定行为
};

// //应该暂不需要
// class Admin_page : public Page
// {
//     int pagestore(User *currentUser, std::vector<User *> &users);
// };

#endif // PAGE_H