#ifndef CLIENT_UI_H
#define CLIENT_UI_H

#include <string>
#include <memory>
#include <vector>
#include "../client/client_api.h"

class ClientUI
{
private:
    ClientAPI api;
    bool isRunning;

    // 通用方法
    void showHeader(const std::string &title);
    void showFooter();
    void pause();
    int getMenuChoice(int min, int max);
    std::string getInput(const std::string &prompt);

    // 主菜单
    void showMainMenu();
    void handleMainMenu();

    // 认证相关
    void showLoginMenu();
    void handleLogin();
    void handleRegister();
    void handleLogout();

    // 产品相关
    void showProductMenu();
    void handleProductList();
    void handleProductSearch();
    void handleProductDetail(int productId);

    // 购物车相关
    void showCartMenu();
    void handleCartList();
    void handleCartAdd();
    void handleCartUpdate();
    void handleCartRemove();

    // 订单相关
    void showOrderMenu();
    void handleOrderList();
    void handleOrderDetail(int orderId);
    void handleOrderCreate();
    void handleOrderPay();
    void handleOrderCancel();

public:
    ClientUI(const std::string &serverHost = "127.0.0.1", int serverPort = 8888);
    ~ClientUI();

    // 启动UI
    void run();

    // 停止UI
    void stop();
};

#endif // CLIENT_UI_H
