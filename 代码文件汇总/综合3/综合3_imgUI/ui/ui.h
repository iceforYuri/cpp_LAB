#ifndef UI_H
#define UI_H

#include <string>
#include <vector>
#include <functional>
#include <memory> // 为shared_ptr添加
#include "../user/user.h"
#include "../store/store.h"
#include "../order/order.h"
#include "../order/ordermanager.h"
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glfw.h"
#include "../imgui/backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>

// 前向声明
class User;
class Store;
class Product;
class Customer;
class Order;
class OrderManager;
class OrderItem;

// UI系统主类
class UI
{
private:
    // 窗口尺寸
    int width = 800;
    int height = 600;

    // UI状态
    bool shouldClose = false;
    int currentPage = 0; // 0=主菜单, 1=用户菜单, 2=商城
    int userType = 0;    // 0=游客, 1=消费者, 2=商家, 3=管理员

    // 商品滚动位置
    float scrollY = 0.0f;

    // 登录输入缓冲区
    char usernameBuffer[128] = {0};
    char passwordBuffer[128] = {0};

    // 用户信息和商店引用
    User *currentUser = nullptr;
    std::vector<User *> *users = nullptr;
    Store *store = nullptr;

    // 商品搜索
    char searchBuffer[128] = {0};
    std::vector<Product *> searchResults;

    // 购买商品
    char buyProductName[128] = {0};
    int buyQuantity = 1;
    // 商品管理
    char newProductName[128] = {0};
    char newProductDesc[256] = {0};
    float newProductPrice = 0.0f;
    int newProductQuantity = 0;
    char newProductAttr1[128] = {0}; // 作者/尺寸/过期日期
    char newProductAttr2[128] = {0}; // ISBN/颜色
    int newProductType = 0;          // 0=图书, 1=服装, 2=食品

    // 充值金额
    float depositAmount = 0.0f;

    // 订单管理
    OrderManager *orderManager = nullptr;
    std::vector<std::shared_ptr<Order>> pendingOrders;
    std::vector<std::shared_ptr<Order>> completedOrders;
    int selectedOrderIndex = -1; // 购物车视图
    bool showCartWindow = false;

    // 订单视图
    bool showOrderWindow = false;

    // UI辅助函数
    void renderMainMenu();
    void renderUserMenu();
    void renderStore();
    void renderUserRegister();
    void renderUserLogin();
    void renderStoreCustomer();
    void renderStoreSeller();
    void renderProductList(const std::vector<Product *> &products, const std::string &title);
    void renderCartWindow();
    void renderOrderManagement();
    void renderOrderDetails(std::shared_ptr<Order> order);
    void processOrders();

    // 加载用户订单
    void loadUserOrders();

public:
    UI(int width = 800, int height = 600);
    ~UI();

    // 初始化UI系统
    bool init();

    // 主循环
    void mainLoop(std::vector<User *> &users, Store &store);

    // 是否应该关闭
    bool isClosing() const { return shouldClose; }
    // 设置当前用户
    void setCurrentUser(User *user);

    // 设置订单管理器
    void setOrderManager(OrderManager *manager);

    // 获取菜单返回值 (兼容原代码)
    int getMenuResult();
};

#endif // UI_H
