#ifndef CLIENT_UI_H
#define CLIENT_UI_H

#include <iostream>
#include <limits>
#include <string>

// 前向声明
class ClientApp;

/**
 * @brief 处理客户端用户界面和输入输出
 */
class ClientUI
{
private:
    ClientApp *m_app;

public:
    /**
     * @brief 构造函数
     * @param app 应用程序实例
     */
    ClientUI(ClientApp *app);

    /**
     * @brief 显示主菜单
     * @return 用户选择
     */
    int showMainMenu() const;

    /**
     * @brief 显示消费者菜单
     * @param username 当前用户名
     * @return 用户选择
     */
    int showCustomerMenu(const std::string &username) const;

    /**
     * @brief 显示商家菜单
     * @param username 当前用户名
     * @return 用户选择
     */
    int showSellerMenu(const std::string &username) const; /**
                                                            * @brief 清除输入缓冲区
                                                            */
    void clearInputBuffer() const;
};

#endif // CLIENT_UI_H
