#include <iostream>
#include <vector>
#include <string>
#include "user/user.h"
#include "store/store.h"
#include "ui/ui.h"

// 文件路径常量
extern const std::string USER_FILE = "./user/users.txt";
extern const std::string STORE_FILE = "./store";

int main()
{
    std::vector<User *> users;
    User *currentUser = nullptr;

    // 创建 Store 对象
    Store store(STORE_FILE);

    // 加载用户数据
    users = User::loadUsersFromFile(USER_FILE);
    std::cout << "已加载 " << users.size() << " 个用户数据。" << std::endl;
    
    // 加载商店数据
    store.loadAllProducts();

    // 创建并初始化UI
    UI ui(800, 600);
    if (!ui.init()) {
        std::cerr << "UI初始化失败，程序退出" << std::endl;
        return 1;
    }
    
    // 启动UI主循环
    ui.mainLoop(users, store);
    
    // 程序结束，释放资源
    for (User* user : users) {
        delete user;
    }
    users.clear();

    return 0;
}
