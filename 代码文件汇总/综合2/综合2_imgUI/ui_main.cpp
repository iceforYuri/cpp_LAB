#include <iostream>
#include <vector>
#include <string>
#include "user/user.h"
#include "store/store.h"
#include "ordermanager/ordermanager.h"
#include "ui/ui.h"
#include <windows.h>

// 文件路径常量
extern const std::string USER_FILE = "./user/users.txt";
extern const std::string STORE_FILE = "./store";
extern const std::string CART_FILE = "./user/carts";
extern const std::string ORDER_DIR = "./order/orders"; // 订单目录
OrderManager g_orderManager(ORDER_DIR);                // 创建全局订单管理器对象

int main()
{
// 设置控制台输出编码为UTF-8
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    std::vector<User *> users;
    User *currentUser = nullptr;

    // 创建 Store 对象
    Store store(STORE_FILE);

    // 加载用户数据
    users = User::loadUsersFromFile(USER_FILE);
    std::cout << "已加载 " << users.size() << " 个用户数据。" << std::endl;
    // 加载商店数据
    store.loadAllProducts(); // 启动订单处理线程
    g_orderManager.startProcessingThread(store, users);
    std::cout << "订单处理线程已启动。" << std::endl;

    // 创建并初始化UI
    UI ui(800, 600);
    ui.setOrderManager(&g_orderManager);
    if (!ui.init())
    {
        std::cerr << "UI初始化失败，程序退出" << std::endl;
        return 1;
    }

    // 启动UI主循环
    ui.mainLoop(users, store); // 停止订单处理线程
    g_orderManager.stopProcessingThread();

    // 保存用户数据
    User::saveUsersToFile(users, USER_FILE);
    std::cout << "用户数据已保存。" << std::endl;

    // 程序结束，释放资源
    for (User *user : users)
    {
        delete user;
    }
    users.clear();

    return 0;
}
