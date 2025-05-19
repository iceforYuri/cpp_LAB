#ifndef SERVER_APP_H
#define SERVER_APP_H

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>
#include <set>
#include <algorithm>
#include <chrono>

#include "user/user.h"
#include "store/store.h"
#include "order/order.h"
#include "ordermanager/ordermanager.h"
#include "client_handler.h"

#pragma comment(lib, "ws2_32.lib")

/**
 * @brief 服务器应用主类，管理服务器的生命周期
 */
class ServerApp
{
private:
    // 网络相关
    SOCKET m_serverSocket;
    bool m_initialized;

    // 资源管理
    std::string m_userFile;
    std::string m_storeFile;
    std::string m_cartFile;
    std::string m_orderDir;

    // 业务数据
    std::vector<User *> m_users;
    Store *m_store;
    OrderManager *m_orderManager;

    // 线程和同步
    std::vector<std::thread> m_clientThreads;
    std::mutex m_usersMutex;
    std::mutex m_storeMutex;
    std::mutex m_loggedInUsersMutex;
    std::atomic<bool> m_serverRunning;
    std::set<std::string> m_loggedInUsers;

public:
    /**
     * @brief 构造函数
     * @param userFile 用户数据文件路径
     * @param storeFile 商店数据文件路径
     * @param cartFile 购物车数据文件路径
     * @param orderDir 订单数据目录路径
     */
    ServerApp(
        const std::string &userFile = "./user/users.txt",
        const std::string &storeFile = "./store",
        const std::string &cartFile = "./user/carts",
        const std::string &orderDir = "./order/orders");

    /**
     * @brief 析构函数
     */
    ~ServerApp();

    /**
     * @brief 初始化服务器
     * @return 是否成功初始化
     */
    bool initialize();

    /**
     * @brief 运行服务器
     * @return 退出代码
     */
    int run();

    /**
     * @brief 停止服务器
     */
    void stop();

    /**
     * @brief 接受客户端连接
     */
    void acceptConnections();

    /**
     * @brief 清理资源
     */
    void cleanup();

    /**
     * @brief 获取用户互斥锁
     * @return 用户互斥锁引用
     */
    std::mutex &getUsersMutex();

    /**
     * @brief 获取商店互斥锁
     * @return 商店互斥锁引用
     */
    std::mutex &getStoreMutex();

    /**
     * @brief 获取已登录用户互斥锁
     * @return 已登录用户互斥锁引用
     */
    std::mutex &getLoggedInUsersMutex();

    /**
     * @brief 获取用户列表
     * @return 用户列表引用
     */
    std::vector<User *> &getUsers();

    /**
     * @brief 获取商店对象
     * @return 商店对象指针
     */
    Store *getStore() const;

    /**
     * @brief 获取订单管理器
     * @return 订单管理器指针
     */
    OrderManager *getOrderManager() const;

    /**
     * @brief 获取已登录用户集合
     * @return 已登录用户集合引用
     */
    std::set<std::string> &getLoggedInUsers();

    /**
     * @brief 服务器是否正在运行
     * @return 是否正在运行
     */
    bool isRunning() const;

    /**
     * @brief 添加已登录用户
     * @param username 用户名
     */
    void addLoggedInUser(const std::string &username);

    /**
     * @brief 移除已登录用户
     * @param username 用户名
     */
    void removeLoggedInUser(const std::string &username);

    /**
     * @brief 检查用户是否已登录
     * @param username 用户名
     * @return 是否已登录
     */
    bool isUserLoggedIn(const std::string &username) const;
};

#endif // SERVER_APP_H
