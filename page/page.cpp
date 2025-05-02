#include "page.h"
#include "../user/user.h"
#include "../store/store.h" // 需要 Store 定义
#include <iostream>
#include <cstdio> // for printf if used
#include <limits>
#include <vector>
#include <string>

using namespace std;

extern const string USER_FILE;
// STORE_FILE is managed by the Store object passed in

// --- 基类 Page 菜单实现 ---
// 这些函数现在只处理用户账户菜单，并在选择商城时返回特定值

int Page::pagemain(vector<User *> &users, User *&currentUser)
{
    int choice = 0;
    while (true)
    {
        printf("\n\n\n--- 主菜单 ---\n");
        cout << "1. 注册新用户" << endl;
        cout << "2. 用户登录" << endl;
        cout << "3. 进入商城 (游客)" << endl; // 返回 3 表示进入商城
        cout << "4. 退出程序" << endl;
        cout << "请选择一个选项: ";
        if (!(cin >> choice))
        {
            cout << "无效输入。" << endl;
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();
        switch (choice)
        {
        case 1:
            currentUser = User::registerUser(users);
            if (currentUser)
                User::saveUsersToFile(users, USER_FILE);
            break; // 注册后留在主菜单或根据需要返回
        case 2:
            currentUser = User::userLogin(users);
            if (currentUser)
                return 1;
            break; // 登录成功，返回 1 给 main 处理
        case 3:
            return 3; // 返回 3，通知 main 调用 pagestore
        case 4:
            cout << "退出程序..." << endl;
            return 0;
        default:
            cout << "无效选项。" << endl;
        }
    }
}

int Page::pagemainCustomer(User *currentUser, vector<User *> &users)
{
    int choice = 0;
    while (true)
    {
        printf("\n\n\n--- 消费者菜单 (%s) ---\n", currentUser->getUsername().c_str());
        cout << "1. 查看个人信息" << endl;
        cout << "2. 修改密码" << endl;
        cout << "3. 查询余额" << endl;
        cout << "4. 充值" << endl;
        cout << "5. 进入商城" << endl; // 返回 3 表示进入商城
        cout << "6. 退出登录" << endl;
        cout << "请选择一个选项: ";
        if (!(cin >> choice))
        {
            cout << "无效输入。" << endl;
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();
        switch (choice)
        {
        case 1:
            currentUser->displayInfo();
            break;
        case 2:
        { /* ... 修改密码 ... */
            string oldP, newP;
            cout << "原密码: ";
            getline(cin >> ws, oldP);
            cout << "新密码: ";
            getline(cin >> ws, newP);
            if (currentUser->changePassword(oldP, newP))
                User::saveUsersToFile(users, USER_FILE);
        }
        break;
        case 3:
            cout << "余额: " << currentUser->checkBalance() << endl;
            break;
        case 4:
        { /* ... 充值 ... */
            double amt;
            cout << "充值金额: ";
            if (!(cin >> amt))
            {
                cout << "无效金额\n";
                clearInputBuffer();
                break;
            }
            clearInputBuffer();
            if (currentUser->deposit(amt))
                User::saveUsersToFile(users, USER_FILE);
        }
        break;
        case 5:
            return 3; // 返回 3，通知 main 调用 pagestore
        case 6:
            cout << "退出登录..." << endl;
            return 2; // 返回 2 给 main 处理退出登录
        default:
            cout << "无效选项。" << endl;
        }
    }
}

int Page::pagemainSeller(User *currentUser, vector<User *> &users)
{
    int choice = 0;
    while (true)
    {
        printf("\n\n\n--- 商家菜单 (%s) ---\n", currentUser->getUsername().c_str());
        cout << "1. 查看商户信息" << endl;
        cout << "2. 修改密码" << endl;
        cout << "3. 进入商城 (管理)" << endl; // 返回 3 表示进入商城
        cout << "4. 退出登录" << endl;
        cout << "请选择一个选项: ";
        if (!(cin >> choice))
        {
            cout << "无效输入。" << endl;
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();
        switch (choice)
        {
        case 1:
            currentUser->displayInfo();
            break;
        case 2:
        { /* ... 修改密码 ... */
            string oldP, newP;
            cout << "原密码: ";
            getline(cin >> ws, oldP);
            cout << "新密码: ";
            getline(cin >> ws, newP);
            if (currentUser->changePassword(oldP, newP))
                User::saveUsersToFile(users, USER_FILE);
        }
        break;
        case 3:
            return 3; // 返回 3，通知 main 调用 pagestore
        case 4:
            cout << "退出登录..." << endl;
            return 2; // 返回 2 给 main 处理退出登录
        default:
            cout << "无效选项。" << endl;
        }
    }
}

int Page::pagemainAdmin(User *currentUser, vector<User *> &users)
{
    // ... (管理员菜单保持不变，不进入普通商城) ...
    int choice = 0;
    while (true)
    {
        printf("\n\n\n--- 管理员菜单 (%s) ---\n", currentUser->getUsername().c_str());
        cout << "1. 查看所有用户信息" << endl;
        cout << "2. 退出登录" << endl;
        cout << "请选择一个选项: ";
        if (!(cin >> choice))
        {
            cout << "无效输入。" << endl;
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();
        switch (choice)
        {
        case 1:
            User::displayAllUsers(users);
            break;
        case 2:
            cout << "退出登录..." << endl;
            return 2;
        default:
            cout << "无效选项。" << endl;
        }
    }
}

// --- Customer_page 商城实现 ---
int Customer_page::pagestore(User *currentUser, std::vector<User *> &users, Store &store)
{
    int choice = -1;
    string username = currentUser ? currentUser->getUsername() : "游客";
    do
    {
        cout << "\n--- 商城 (用户: " << username << ") ---" << endl;
        cout << "1. 显示所有商品" << endl;
        cout << "2. 按名称搜索商品" << endl;
        // 可以添加购买功能
        cout << "0. 退出商城" << endl;
        cout << "请选择: ";
        if (!(cin >> choice))
        {
            cout << "无效输入。" << endl;
            clearInputBuffer();
            choice = -1;
            continue;
        }
        clearInputBuffer();

        switch (choice)
        {
        case 1:
            store.displayAllProducts();
            break;
        case 2:
        {
            string searchTerm;
            cout << "输入搜索名称: ";
            getline(cin >> ws, searchTerm);
            vector<Product *> results = store.searchProductsByName(searchTerm);
            if (results.empty())
            {
                cout << "未找到匹配商品。" << endl;
            }
            else
            {
                cout << "\n--- 搜索结果 ---" << endl;
                for (const auto *p : results)
                {
                    cout << "-----------------" << endl;
                    p->display();
                }
                cout << "-----------------" << endl;
            }
            break;
        }
        case 0:
            cout << "正在退出商城..." << endl;
            break;
        default:
            cout << "无效选项。" << endl;
        }
    } while (choice != 0);
    return 1; // 返回 1 表示正常退出商城，继续主循环
}

// --- Seller_page 商城实现 ---
int Seller_page::pagestore(User *currentUser, std::vector<User *> &users, Store &store)
{
    // 确保当前用户是商家
    if (!currentUser || currentUser->getUserType() != "商家")
    {
        cout << "错误：只有商家才能访问此页面。" << endl;
        return 1; // 直接退出
    }

    int choice = -1;
    do
    {
        cout << "\n--- 商城管理 (商家: " << currentUser->getUsername() << ") ---" << endl;
        cout << "1. 显示所有商品" << endl;
        cout << "2. 按名称搜索商品" << endl;
        cout << "3. 添加新商品" << endl;
        cout << "4. 修改商品价格" << endl;
        cout << "5. 修改商品库存" << endl;
        cout << "6. 修改商品折扣" << endl;
        cout << "7. 应用分类折扣" << endl;
        cout << "0. 退出商城管理" << endl;
        cout << "请选择: ";
        if (!(cin >> choice))
        {
            cout << "无效输入。" << endl;
            clearInputBuffer();
            choice = -1;
            continue;
        }
        clearInputBuffer();

        switch (choice)
        {
        case 1:
            store.displayAllProducts();
            break;
        case 2:
        {
            string searchTerm;
            cout << "输入搜索名称: ";
            getline(cin >> ws, searchTerm);
            vector<Product *> results = store.searchProductsByName(searchTerm);
            if (results.empty())
            {
                cout << "未找到匹配商品。" << endl;
            }
            else
            {
                cout << "\n--- 搜索结果 ---" << endl;
                for (const auto *p : results)
                {
                    cout << "-----------------" << endl;
                    p->display();
                }
                cout << "-----------------" << endl;
            }
            break;
        }
        case 3:
        {
            // --- 添加商品逻辑 ---
            // 需要收集所有商品信息，然后调用 store.addProduct 或 store.createBook 等
            cout << "添加商品功能需要实现：收集信息并调用 Store 方法。" << endl;
            // 示例：添加图书
            string name, desc, author, isbn;
            double price;
            int qty;
            cout << "输入书名: ";
            getline(cin >> ws, name);
            // ... 收集其他信息 ...
            // if (store.createBook(currentUser, name, desc, price, qty, author, isbn)) {
            //     cout << "添加成功！" << endl;
            // } else {
            //     cout << "添加失败！" << endl;
            // }
            break;
        }
        case 4:
        { // 修改价格
            string name;
            double price;
            cout << "输入商品名称: ";
            getline(cin >> ws, name);
            cout << "输入新价格: ";
            if (!(cin >> price))
            {
                cout << "无效价格\n";
                clearInputBuffer();
                break;
            }
            clearInputBuffer();
            if (store.manageProductPrice(currentUser, name, price))
                cout << "修改成功\n";
            else
                cout << "修改失败\n";
            break;
        }
        case 5:
        { // 修改库存
            string name;
            int qty;
            cout << "输入商品名称: ";
            getline(cin >> ws, name);
            cout << "输入新库存: ";
            if (!(cin >> qty))
            {
                cout << "无效库存\n";
                clearInputBuffer();
                break;
            }
            clearInputBuffer();
            if (store.manageProductQuantity(currentUser, name, qty))
                cout << "修改成功\n";
            else
                cout << "修改失败\n";
            break;
        }
        case 6:
        { // 修改折扣
            string name;
            double discount;
            cout << "输入商品名称: ";
            getline(cin >> ws, name);
            cout << "输入新折扣 (0.0-1.0): ";
            if (!(cin >> discount))
            {
                cout << "无效折扣\n";
                clearInputBuffer();
                break;
            }
            clearInputBuffer();
            if (store.manageProductDiscount(currentUser, name, discount))
                cout << "修改成功\n";
            else
                cout << "修改失败\n";
            break;
        }
        case 7:
        { // 应用分类折扣
            string category;
            double discount;
            cout << "输入分类 (Book, Clothing, Food): ";
            getline(cin >> ws, category);
            cout << "输入折扣 (0.0-1.0): ";
            if (!(cin >> discount))
            {
                cout << "无效折扣\n";
                clearInputBuffer();
                break;
            }
            clearInputBuffer();
            if (store.applyCategoryDiscount(currentUser, category, discount))
                cout << "应用成功\n";
            else
                cout << "应用失败或无商品\n";
            break;
        }
        case 0:
            cout << "正在退出商城管理..." << endl;
            break;
        default:
            cout << "无效选项。" << endl;
        }
    } while (choice != 0);
    return 1; // 返回 1 表示正常退出商城，继续主循环
}