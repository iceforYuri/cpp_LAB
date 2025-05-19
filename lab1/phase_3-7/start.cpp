#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include "user/user.h"
#include "page/page.h"   // 包含 Page 基类和派生类
#include "store/store.h" // 包含 Store 类
#include "ordermanager/ordermanager.h"
#include <cmath>
#include <windows.h> // Windows 特有的头文件，用于 Sleep 函数

using namespace std;

// 文件路径常量
extern const string USER_FILE = "./user/users.txt";
extern const string STORE_FILE = "./store";
extern const string CART_FILE = "./user/carts";
extern const string ORDER_DIR = "./order/orders"; // 订单目录
bool startProcessing;                             // 用于控制订单处理线程的启动状态

OrderManager g_orderManager(ORDER_DIR); // 创建订单管理器对象

int main()
{
    SetConsoleOutputCP(65001); // 设置控制台输出编码为 UTF-8
    SetConsoleCP(65001);       // 设置控制台输入编码为 UTF-8

    vector<User *> users;
    User *currentUser = nullptr;

    // 创建 Store 对象
    Store store(STORE_FILE);

    // 创建具体的 Page 派生类对象
    Customer_page customerPage;
    Seller_page sellerPage;
    // Page* currentPageManager = &customerPage; // 如果需要基类指针进行多态调用

    // 加载用户数据
    users = User::loadUsersFromFile(USER_FILE);
    cout << "已加载 " << users.size() << " 个用户数据。" << endl;

    // 启动订单处理线程
    // startProcessing = false;
    g_orderManager.startProcessingThread(store, users);
    // cout << "订单处理线程已启动。" << endl;

    int menuResult = 1; // 1=继续, 0=退出程序, 2=退出登录, 3=进入商城

    while (menuResult != 0)
    {
        // if (g_orderManager.getPendingOrderCount() > 0)
        // {
        //     g_orderManager.processNextOrder(store, users);
        // }
        // 根据 menuResult 处理状态
        if (menuResult == 2)
        { // 处理退出登录
            currentUser = nullptr;
            menuResult = 1;
        }
        if (menuResult == 3)
        { // 处理进入商城
            if (currentUser == nullptr || dynamic_cast<Customer *>(currentUser))
            {
                menuResult = customerPage.pagestore(currentUser, users, store);
            }
            else if (dynamic_cast<Seller *>(currentUser))
            {
                menuResult = sellerPage.pagestore(currentUser, users, store);
            }
            else
            {
                cout << "当前用户类型无法进入商城。" << endl;
                menuResult = 1;
            }
            if (menuResult != 0 && menuResult != 2)
                menuResult = 1; // 确保返回后继续菜单循环
            continue;
        }

        // 显示用户账户菜单
        if (currentUser == nullptr) // 未登录
        {
            // 使用 customerPage 对象显示主菜单
            menuResult = customerPage.pagemain(users, currentUser);
        }
        else // 已登录
        {
            if (dynamic_cast<Customer *>(currentUser))
            {
                menuResult = customerPage.pagemainCustomer(currentUser, users);
            }
            else if (dynamic_cast<Seller *>(currentUser))
            {
                menuResult = sellerPage.pagemainSeller(currentUser, users);
            }
            // else if (dynamic_cast<Admin*>(currentUser)) { ... }
            else // 使用字符串匹配作为后备
            {
                string type = currentUser->getUserType();
                if (type == "消费者")
                {
                    menuResult = customerPage.pagemainCustomer(currentUser, users);
                }
                else if (type == "商家")
                {
                    menuResult = sellerPage.pagemainSeller(currentUser, users);
                }
                // else if (type == "管理员") { ... }
                else
                {
                    cout << "未知的用户类型 (" << type << ")，将退出登录。" << endl;
                    menuResult = 2;
                }
            }
        }
    } // End while loop

    // ... (程序结束部分保持不变) ...
    User::saveUsersToFile(users, USER_FILE);
    cout << "用户数据已保存。" << endl;
    for (auto *user : users)
    {
        delete user;
    }
    users.clear();
    return 0;
}