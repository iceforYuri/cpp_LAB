#include "client/client.h"

/**
 * @file client_run.cpp
 * @brief 电子商城系统客户端入口点
 *
 * 本文件提供了电子商城系统客户端的入口点，实例化并运行Client类。
 * 该程序通过重构，将原来使用全局变量和函数的设计转变为面向对象的设计。
 */

int main()
{
    // 创建客户端实例
    Client client;

    // 初始化客户端
    if (!client.initialize())
    {
        std::cerr << "客户端初始化失败" << std::endl;
        return 1;
    }

    // 运行客户端
    client.run();

    // 退出程序
    return 0;
}