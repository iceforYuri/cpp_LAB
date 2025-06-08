#include "ui.h"
#include "../user/user.h"
#include "../store/store.h"
#include "../order/order.h"
#include "../ordermanager/ordermanager.h"
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glfw.h"
#include "../imgui/backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <string>
#include <memory>

// 全局GLFW窗口指针
GLFWwindow *window = nullptr;

// UI构造函数
UI::UI(int width, int height)
    : width(width), height(height) {}

// UI析构函数
UI::~UI()
{
    // 清理ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // 清理GLFW
    if (window)
    {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

// 设置订单管理器
void UI::setOrderManager(OrderManager *manager)
{
    orderManager = manager;
}

// 初始化UI
bool UI::init()
{
    // 初始化GLFW
    if (!glfwInit())
    {
        std::cerr << "GLFW初始化失败" << std::endl;
        return false;
    }

    // 创建窗口
    window = glfwCreateWindow(width, height, "电子商城系统", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "创建窗口失败" << std::endl;
        glfwTerminate();
        return false;
    }

    // 设置当前上下文
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // 启用垂直同步

    // 初始化ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 启用键盘控制
    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\simhei.ttf", 16.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());

    // 设置ImGui风格
    ImGui::StyleColorsLight();

    // 平台/渲染器后端
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    return true;
}

// 设置当前用户
void UI::setCurrentUser(User *user)
{
    currentUser = user;
    if (user)
    { // 设置用户类型
        std::string type = user->getUserType();
        if (type == "customer")
        {
            userType = 1;

            // 加载购物车
            Customer *customer = dynamic_cast<Customer *>(user);
            if (customer)
            {
                customer->loadCartFromFile();
            }

            // 用户登录时不自动显示订单窗口
            showOrderWindow = false;
        }
        else if (type == "seller")
            userType = 2;
        else if (type == "admin")
            userType = 3;
        else
            userType = 0;
    }
    else
    {
        userType = 0;
    }
}

// 主循环
void UI::mainLoop(std::vector<User *> &usersRef, Store &storeRef)
{
    users = &usersRef;
    store = &storeRef; // 主循环
    while (!glfwWindowShouldClose(window) && !shouldClose)
    {
        glfwPollEvents();

        // 开始新帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 全屏窗口
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(width, height));
        ImGui::Begin("商城系统", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoScrollbar);

        // 处理订单队列
        if (orderManager)
        {
            processOrders();
        }

        // 根据当前页面渲染不同的UI
        switch (currentPage)
        {
        case 0: // 主菜单
            renderMainMenu();
            break;
        case 1: // 用户菜单
            renderUserMenu();
            break;
        case 2: // 商城
            renderStore();
            break;
        case 3: // 注册页面
            renderUserRegister();
            break;
        case 4: // 登录页面
            renderUserLogin();
            break;
        } // 显示购物车窗口
        if (showCartWindow && currentUser && userType == 1)
        {
            renderCartWindow();
        }

        // 显示订单管理窗口
        if (showOrderWindow && currentUser && userType == 1 && orderManager)
        {
            renderOrderManagement();
        }

        ImGui::End();

        // 渲染
        ImGui::Render();
        int displayW, displayH;
        glfwGetFramebufferSize(window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

// 渲染主菜单
void UI::renderMainMenu()
{
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("电子商城系统");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::SetCursorPosX((width - 200) / 2);
    if (ImGui::Button("注册新用户", ImVec2(200, 50)))
    {
        currentPage = 3; // 切换到注册页面
    }

    ImGui::Spacing();
    ImGui::SetCursorPosX((width - 200) / 2);
    if (ImGui::Button("用户登录", ImVec2(200, 50)))
    {
        currentPage = 4; // 切换到登录页面
        memset(usernameBuffer, 0, sizeof(usernameBuffer));
        memset(passwordBuffer, 0, sizeof(passwordBuffer));
    }

    ImGui::Spacing();
    ImGui::SetCursorPosX((width - 200) / 2);
    if (ImGui::Button("进入商城 (游客)", ImVec2(200, 50)))
    {
        currentPage = 2; // 切换到商城
    }

    ImGui::Spacing();
    ImGui::SetCursorPosX((width - 200) / 2);
    if (ImGui::Button("退出程序", ImVec2(200, 50)))
    {
        shouldClose = true;
    }
}

// 渲染用户菜单
void UI::renderUserMenu()
{
    if (!currentUser)
    {
        currentPage = 0; // 回到主菜单
        return;
    }

    std::string userTypeStr;
    if (userType == 1)
        userTypeStr = "消费者";
    else if (userType == 2)
        userTypeStr = "商家";
    else if (userType == 3)
        userTypeStr = "管理员";
    else
        userTypeStr = "未知";

    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("%s菜单 - %s", userTypeStr.c_str(), currentUser->getUsername().c_str());
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();

    // 用户信息
    ImGui::BeginChild("UserInfo", ImVec2(width * 0.4f, height - 150), true);
    ImGui::Text("用户名: %s", currentUser->getUsername().c_str());
    ImGui::Text("用户类型: %s", userTypeStr.c_str());
    ImGui::Text("账户余额: CNY%.2f", currentUser->checkBalance());
    ImGui::EndChild();

    // 操作区域
    ImGui::SameLine();
    ImGui::BeginChild("UserActions", ImVec2(0, height - 150), true);

    // 修改密码
    static char oldPass[128] = {0};
    static char newPass[128] = {0};
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("原密码", oldPass, IM_ARRAYSIZE(oldPass), ImGuiInputTextFlags_Password);
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("新密码", newPass, IM_ARRAYSIZE(newPass), ImGuiInputTextFlags_Password);
    if (ImGui::Button("修改密码", ImVec2(100, 30)))
    {
        if (currentUser->changePassword(oldPass, newPass))
        {
            User::saveUsersToFile(*users, "user/users.txt");
            ImGui::OpenPopup("密码修改成功");
            memset(oldPass, 0, sizeof(oldPass));
            memset(newPass, 0, sizeof(newPass));
        }
        else
        {
            ImGui::OpenPopup("密码修改失败");
        }
    }

    // 弹窗
    if (ImGui::BeginPopupModal("密码修改成功", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("密码修改成功！");
        if (ImGui::Button("确定", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("密码修改失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("密码修改失败，请检查原密码是否正确。");
        if (ImGui::Button("确定", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    // 消费者特有 - 充值
    if (userType == 1)
    {
        ImGui::SetNextItemWidth(200);
        ImGui::InputFloat("充值金额", &depositAmount, 10.0f, 100.0f, "%.2f");
        if (ImGui::Button("充值", ImVec2(100, 30)))
        {
            if (depositAmount > 0)
            {
                if (currentUser->deposit(depositAmount))
                {
                    User::saveUsersToFile(*users, "user/users.txt");
                    ImGui::OpenPopup("充值成功");
                    depositAmount = 0.0f;
                }
            }
            else
            {
                ImGui::OpenPopup("充值失败");
            }
        }

        if (ImGui::BeginPopupModal("充值成功", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("充值成功！当前余额: CNY%.2f", currentUser->checkBalance());
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("充值失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("充值失败，请确保充值金额大于0。");
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::EndChild();

    // 底部按钮
    ImGui::Separator();

    if (ImGui::Button("进入商城", ImVec2(120, 40)))
    {
        currentPage = 2; // 切换到商城
        searchResults.clear();
        memset(searchBuffer, 0, sizeof(searchBuffer));
    }

    ImGui::SameLine();
    if (ImGui::Button("退出登录", ImVec2(120, 40)))
    {
        currentUser = nullptr;
        userType = 0;
        currentPage = 0; // 回到主菜单
    }
}

// 渲染商城
void UI::renderStore()
{
    ImGui::SetWindowFontScale(1.5f);
    if (currentUser)
    {
        if (userType == 2)
        {
            ImGui::Text("商城管理 - %s", currentUser->getUsername().c_str());
        }
        else
        {
            ImGui::Text("商城 - %s", currentUser->getUsername().c_str());
        }
    }
    else
    {
        ImGui::Text("商城 - 游客模式");
    }
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();

    // 根据用户类型显示不同的商城界面
    if (userType == 2)
    { // 商家
        renderStoreSeller();
    }
    else
    { // 消费者或游客
        renderStoreCustomer();
    }

    // 底部按钮
    ImGui::Separator();
    if (ImGui::Button("返回", ImVec2(120, 40)))
    {
        if (currentUser)
        {
            currentPage = 1; // 返回用户菜单
        }
        else
        {
            currentPage = 0; // 返回主菜单
        }
    }
}

// 渲染注册页面
void UI::renderUserRegister()
{
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("用户注册");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();

    static char regUsername[128] = {0};
    static char regPassword[128] = {0};
    static int regUserType = 0; // 0=消费者, 1=商家

    ImGui::SetNextItemWidth(250);
    ImGui::InputText("用户名", regUsername, IM_ARRAYSIZE(regUsername));
    ImGui::SetNextItemWidth(250);
    ImGui::InputText("密码", regPassword, IM_ARRAYSIZE(regPassword), ImGuiInputTextFlags_Password);
    ImGui::RadioButton("消费者", &regUserType, 0);
    ImGui::SameLine();
    ImGui::RadioButton("商家", &regUserType, 1);

    if (ImGui::Button("注册", ImVec2(120, 40)))
    {
        if (strlen(regUsername) == 0)
        {
            ImGui::OpenPopup("注册失败");
        }
        else if (User::isUsernameExists(*users, regUsername))
        {
            ImGui::OpenPopup("用户名已存在");
        }
        else
        {
            User *newUser = nullptr;
            if (regUserType == 0)
            {
                extern const std::string CART_FILE; // 引用外部声明的购物车目录常量
                newUser = new Customer(regUsername, regPassword, CART_FILE);
            }
            else
            {
                newUser = new Seller(regUsername, regPassword);
            }

            users->push_back(newUser);
            User::saveUsersToFile(*users, "user/users.txt");

            currentUser = newUser;
            setCurrentUser(newUser);
            currentPage = 1; // 跳转至用户菜单

            // 清空输入
            memset(regUsername, 0, sizeof(regUsername));
            memset(regPassword, 0, sizeof(regPassword));
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("返回", ImVec2(120, 40)))
    {
        currentPage = 0; // 回到主菜单
    }

    // 弹窗
    if (ImGui::BeginPopupModal("注册失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("注册失败，用户名不能为空！");
        if (ImGui::Button("确定", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("用户名已存在", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("该用户名已存在，请选择其他用户名！");
        if (ImGui::Button("确定", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

// 渲染登录页面
void UI::renderUserLogin()
{
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("用户登录");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();

    ImGui::SetNextItemWidth(250);
    ImGui::InputText("用户名", usernameBuffer, IM_ARRAYSIZE(usernameBuffer));
    ImGui::SetNextItemWidth(250);
    ImGui::InputText("密码", passwordBuffer, IM_ARRAYSIZE(passwordBuffer), ImGuiInputTextFlags_Password);

    if (ImGui::Button("登录", ImVec2(120, 40)))
    {
        User *user = User::login(*users, usernameBuffer, passwordBuffer);
        if (user)
        {
            currentUser = user;
            setCurrentUser(user);
            currentPage = 1; // 跳转至用户菜单
        }
        else
        {
            ImGui::OpenPopup("登录失败");
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("返回", ImVec2(120, 40)))
    {
        currentPage = 0; // 回到主菜单
    }

    // 弹窗
    if (ImGui::BeginPopupModal("登录失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("登录失败，用户名或密码错误！");
        if (ImGui::Button("确定", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

// 渲染消费者商城
void UI::renderStoreCustomer()
{
    // 增加高度以容纳购物车和订单按钮
    ImGui::BeginChild("StoreControls", ImVec2(width, 100));
    ImGui::SetNextItemWidth(300);
    if (ImGui::InputText("搜索商品", searchBuffer, IM_ARRAYSIZE(searchBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        searchResults = store->searchProductsByName(searchBuffer);
    }
    ImGui::SameLine();
    if (ImGui::Button("搜索", ImVec2(80, 0)))
    {
        searchResults = store->searchProductsByName(searchBuffer);
    }
    ImGui::SameLine();
    if (ImGui::Button("显示所有商品", ImVec2(150, 0)))
    {
        searchResults.clear();
        memset(searchBuffer, 0, sizeof(searchBuffer));
    }

    // 将购物车和订单按钮放在新的一行
    if (currentUser && userType == 1)
    {
        ImGui::Spacing();
        ImGui::Spacing(); // 增加一点垂直间距

        // 居中放置按钮
        float buttonsWidth = 150.0f + 20.0f + 150.0f; // 两个按钮加上它们之间的间距
        ImGui::SetCursorPosX((width - buttonsWidth) * 0.5f);

        if (ImGui::Button("查看购物车", ImVec2(150, 30))) // 增加按钮高度使其更明显
        {
            showCartWindow = true;
        }

        ImGui::SameLine(0, 20.0f);                      // 设置按钮之间的间距为20
        if (ImGui::Button("查看订单", ImVec2(150, 30))) // 和购物车按钮保持一致的大小
        {
            // 加载订单数据
            if (orderManager)
            {
                // 加载用户订单
                loadUserOrders();

                // 设置显示订单窗口标志
                showOrderWindow = true;
            }
        }
    }
    ImGui::EndChild();

    ImGui::Separator();

    // 商品列表
    if (!searchResults.empty())
    {
        renderProductList(searchResults, "搜索结果");
    }
    else
    {
        renderProductList(store->getProducts(), "所有商品");
    } // 购买商品区域
    if (currentUser && userType == 1)
    {
        ImGui::Separator();
        ImGui::Text("购买商品");

        ImGui::SetNextItemWidth(250);
        ImGui::InputText("商品名称", buyProductName, IM_ARRAYSIZE(buyProductName));
        ImGui::SetNextItemWidth(100);
        ImGui::InputInt("购买数量", &buyQuantity);
        if (buyQuantity < 1)
            buyQuantity = 1;

        if (ImGui::Button("直接购买", ImVec2(100, 30)))
        {
            if (strlen(buyProductName) == 0)
            {
                ImGui::OpenPopup("购买失败");
            }
            else
            {
                Product *product = store->findProductByName(buyProductName);
                if (!product)
                {
                    ImGui::OpenPopup("商品不存在");
                }
                else if (product->getQuantity() < buyQuantity)
                {
                    ImGui::OpenPopup("库存不足");
                }
                else
                {
                    double totalCost = product->getPrice() * buyQuantity;
                    if (currentUser->checkBalance() < totalCost)
                    {
                        ImGui::OpenPopup("余额不足");
                    }
                    else
                    {
                        // 扣除用户余额
                        currentUser->withdraw(totalCost);

                        // 添加给商家余额
                        std::string sellerName = product->getSellerUsername();
                        if (!sellerName.empty())
                        {
                            User *seller = User::findUser(*users, sellerName);
                            if (seller)
                            {
                                seller->deposit(totalCost);
                            }
                        }

                        // 减少库存
                        product->setQuantity(product->getQuantity() - buyQuantity);

                        // 保存更改
                        User::saveUsersToFile(*users, "user/users.txt");
                        store->saveAllProducts();

                        ImGui::OpenPopup("购买成功");
                        memset(buyProductName, 0, sizeof(buyProductName));
                        buyQuantity = 1;
                    }
                }
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("加入购物车", ImVec2(120, 30)))
        {
            if (strlen(buyProductName) == 0)
            {
                ImGui::OpenPopup("购买失败");
            }
            else
            {
                Product *product = store->findProductByName(buyProductName);
                if (!product)
                {
                    ImGui::OpenPopup("商品不存在");
                }
                else if (product->getQuantity() < buyQuantity)
                {
                    ImGui::OpenPopup("库存不足");
                }
                else
                {
                    // 转换为Customer*
                    Customer *customer = dynamic_cast<Customer *>(currentUser);
                    if (customer)
                    {
                        // 添加到购物车
                        if (customer->addToCart(*product, buyQuantity))
                        {
                            ImGui::OpenPopup("加入购物车成功");
                            memset(buyProductName, 0, sizeof(buyProductName));
                            buyQuantity = 1;
                        }
                        else
                        {
                            ImGui::OpenPopup("加入购物车失败");
                        }
                    }
                }
            }
        }

        // 弹窗
        if (ImGui::BeginPopupModal("购买失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("请输入要购买的商品名称！");
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("商品不存在", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("找不到商品 \"%s\"！", buyProductName);
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("库存不足", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            Product *product = store->findProductByName(buyProductName);
            if (product)
            {
                ImGui::Text("库存不足！当前库存: %d", product->getQuantity());
            }
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("余额不足", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            Product *product = store->findProductByName(buyProductName);
            if (product)
            {
                double totalCost = product->getPrice() * buyQuantity;
                ImGui::Text("余额不足！需要: CNY%.2f, 当前余额: CNY%.2f",
                            totalCost, currentUser->checkBalance());
            }
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("购买成功", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("购买成功！当前余额: CNY%.2f", currentUser->checkBalance());
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("加入购物车成功", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("成功加入购物车！");
            if (ImGui::Button("查看购物车", ImVec2(120, 0)))
            {
                showCartWindow = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("继续购物", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("加入购物车失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("加入购物车失败，请稍后重试。");
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

// 渲染商家商城
void UI::renderStoreSeller()
{
    if (!currentUser || userType != 2)
        return;

    std::string sellerName = currentUser->getUsername();

    // 显示选项卡
    static int currentTab = 0;
    ImGui::BeginTabBar("SellerTabs");

    if (ImGui::BeginTabItem("我的商品"))
    {
        ImGui::BeginChild("MyProducts", ImVec2(0, height - 200));
        renderProductList(store->getSellerProducts(sellerName), "我的商品");
        ImGui::EndChild();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("添加商品"))
    {
        ImGui::BeginChild("AddProduct", ImVec2(0, height - 200));

        ImGui::SetNextItemWidth(250);
        ImGui::InputText("商品名称", newProductName, IM_ARRAYSIZE(newProductName));

        ImGui::SetNextItemWidth(350);
        ImGui::InputTextMultiline("商品描述", newProductDesc, IM_ARRAYSIZE(newProductDesc));

        ImGui::SetNextItemWidth(150);
        ImGui::InputFloat("商品价格", &newProductPrice, 1.0f, 10.0f, "CNY%.2f");
        if (newProductPrice < 0)
            newProductPrice = 0;

        ImGui::SetNextItemWidth(150);
        ImGui::InputInt("库存数量", &newProductQuantity);
        if (newProductQuantity < 0)
            newProductQuantity = 0;

        const char *productTypes[] = {"图书", "服装", "食品", "其他"};
        ImGui::SetNextItemWidth(150);
        ImGui::Combo("商品类型", &newProductType, productTypes, IM_ARRAYSIZE(productTypes));

        // 根据不同的商品类型显示不同的输入字段
        if (newProductType == 0)
        { // 图书
            ImGui::SetNextItemWidth(250);
            ImGui::InputText("作者", newProductAttr1, IM_ARRAYSIZE(newProductAttr1));
            ImGui::SetNextItemWidth(250);
            ImGui::InputText("ISBN", newProductAttr2, IM_ARRAYSIZE(newProductAttr2));
        }
        else if (newProductType == 1)
        { // 服装
            ImGui::SetNextItemWidth(150);
            ImGui::InputText("尺寸", newProductAttr1, IM_ARRAYSIZE(newProductAttr1));
            ImGui::SetNextItemWidth(150);
            ImGui::InputText("颜色", newProductAttr2, IM_ARRAYSIZE(newProductAttr2));
        }
        else if (newProductType == 2)
        { // 食品
            ImGui::SetNextItemWidth(150);
            ImGui::InputText("过期日期", newProductAttr1, IM_ARRAYSIZE(newProductAttr1));
        }
        else
        {
            ImGui::SetNextItemWidth(250);
            ImGui::InputText("分类名称", newProductAttr1, IM_ARRAYSIZE(newProductAttr1));
        }

        if (ImGui::Button("添加商品", ImVec2(120, 30)))
        {
            if (strlen(newProductName) == 0)
            {
                ImGui::OpenPopup("商品添加失败");
            }
            else
            {
                bool success = false;

                if (newProductType == 0)
                { // 图书
                    success = store->createBook(
                        currentUser, newProductName, newProductDesc,
                        newProductPrice, newProductQuantity,
                        newProductAttr1, newProductAttr2);
                }
                else if (newProductType == 1)
                { // 服装
                    success = store->createClothing(
                        currentUser, newProductName, newProductDesc,
                        newProductPrice, newProductQuantity,
                        newProductAttr1, newProductAttr2);
                }
                else if (newProductType == 2)
                { // 食品
                    success = store->createFood(
                        currentUser, newProductName, newProductDesc,
                        newProductPrice, newProductQuantity,
                        newProductAttr1);
                }
                else
                { // 其他
                    success = store->createGenericProduct(
                        currentUser, newProductName, newProductDesc,
                        newProductPrice, newProductQuantity,
                        newProductAttr1);
                }

                if (success)
                {
                    ImGui::OpenPopup("商品添加成功");
                    memset(newProductName, 0, sizeof(newProductName));
                    memset(newProductDesc, 0, sizeof(newProductDesc));
                    memset(newProductAttr1, 0, sizeof(newProductAttr1));
                    memset(newProductAttr2, 0, sizeof(newProductAttr2));
                    newProductPrice = 0.0f;
                    newProductQuantity = 0;
                }
                else
                {
                    ImGui::OpenPopup("商品添加失败");
                }
            }
        }
        if (ImGui::BeginPopupModal("商品添加成功", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("商品添加成功！");
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::EndChild();
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("管理商品"))
    {
        ImGui::BeginChild("ManageProducts", ImVec2(0, height - 200));

        ImGui::Text("修改商品价格");
        static char priceProductName[128] = {0};
        static float newPrice = 0.0f;
        ImGui::SetNextItemWidth(250);
        ImGui::InputText("商品名称##price", priceProductName, IM_ARRAYSIZE(priceProductName));
        ImGui::SetNextItemWidth(150);
        ImGui::InputFloat("新价格", &newPrice, 1.0f, 10.0f, "CNY%.2f");
        if (newPrice < 0)
            newPrice = 0;
        if (ImGui::Button("修改价格", ImVec2(120, 30)))
        {
            if (strlen(priceProductName) == 0)
            {
                ImGui::OpenPopup("修改失败");
            }
            else
            {
                if (store->manageProductPrice(currentUser, priceProductName, newPrice))
                {
                    ImGui::OpenPopup("价格修改成功");
                    memset(priceProductName, 0, sizeof(priceProductName));
                    newPrice = 0.0f;
                }
                else
                {
                    ImGui::OpenPopup("价格修改失败");
                }
            }
        }
        if (ImGui::BeginPopupModal("修改失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("请输入商品名称！");
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("价格修改成功", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("商品价格修改成功！");
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("价格修改失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("价格修改失败！请确认商品名称正确且为您所有。");
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Separator();

        ImGui::Text("修改商品库存");
        static char qtyProductName[128] = {0};
        static int newQty = 0;
        ImGui::SetNextItemWidth(250);
        ImGui::InputText("商品名称##qty", qtyProductName, IM_ARRAYSIZE(qtyProductName));
        ImGui::SetNextItemWidth(150);
        ImGui::InputInt("新库存", &newQty);
        if (newQty < 0)
            newQty = 0;
        if (ImGui::Button("修改库存", ImVec2(120, 30)))
        {
            if (strlen(qtyProductName) == 0)
            {
                ImGui::OpenPopup("修改失败");
            }
            else
            {
                if (store->manageProductQuantity(currentUser, qtyProductName, newQty))
                {
                    ImGui::OpenPopup("库存修改成功");
                    memset(qtyProductName, 0, sizeof(qtyProductName));
                    newQty = 0;
                }
                else
                {
                    ImGui::OpenPopup("库存修改失败");
                }
            }
        }

        if (ImGui::BeginPopupModal("库存修改成功", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("商品库存修改成功！");
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("库存修改失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("库存修改失败！请确认商品名称正确且为您所有。");
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Separator();

        ImGui::Text("修改商品折扣");
        static char discountProductName[128] = {0};
        static float newDiscount = 0.0f;
        ImGui::SetNextItemWidth(250);
        ImGui::InputText("商品名称##discount", discountProductName, IM_ARRAYSIZE(discountProductName));
        ImGui::SetNextItemWidth(150);
        ImGui::SliderFloat("折扣 (%)", &newDiscount, 0.0f, 100.0f, "%.1f%%");
        if (ImGui::Button("应用折扣", ImVec2(120, 30)))
        {
            if (strlen(discountProductName) == 0)
            {
                ImGui::OpenPopup("修改失败");
            }
            else
            {
                if (store->manageProductDiscount(currentUser, discountProductName, newDiscount / 100.0f))
                {
                    ImGui::OpenPopup("折扣修改成功");
                    memset(discountProductName, 0, sizeof(discountProductName));
                    newDiscount = 0.0f;
                }
                else
                {
                    ImGui::OpenPopup("折扣修改失败");
                }
            }
        }
        if (ImGui::BeginPopupModal("折扣修改成功", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("商品折扣修改成功！");
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("折扣修改失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("折扣修改失败！请确认商品名称正确且为您所有。");
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Separator();
        ImGui::Text("修改商品类型折扣");
        static char categoryName[128] = {0};
        static float categoryDiscount = 0.0f;
        static int selectedCategory = 0;
        const char *categories[] = {"Book", "Clothing", "Food", "其他"};

        ImGui::SetNextItemWidth(250);
        ImGui::Combo("商品类型", &selectedCategory, categories, IM_ARRAYSIZE(categories));

        // 如果选择"其他"类型，则显示输入框
        if (selectedCategory == 3)
        { // "其他"选项的索引
            ImGui::SetNextItemWidth(250);
            ImGui::InputText("自定义类型", categoryName, IM_ARRAYSIZE(categoryName));
        }

        ImGui::SetNextItemWidth(150);
        ImGui::SliderFloat("类别折扣 (%)", &categoryDiscount, 0.0f, 100.0f, "%.1f%%");
        if (ImGui::Button("应用类别折扣", ImVec2(140, 30)))
        {
            std::string category;
            if (selectedCategory == 3 && strlen(categoryName) > 0)
            {
                // 如果选择"其他"且输入了自定义类型
                category = categoryName;
            }
            else
            {
                category = categories[selectedCategory];
            }

            if (store->applyCategoryDiscount(currentUser, category, categoryDiscount / 100.0f))
            {
                ImGui::OpenPopup("类别折扣修改成功");
                categoryDiscount = 0.0f;
                if (selectedCategory == 3)
                {
                    memset(categoryName, 0, sizeof(categoryName));
                }
            }
            else
            {
                ImGui::OpenPopup("类别折扣修改失败");
            }
        }
        if (ImGui::BeginPopupModal("类别折扣修改成功", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("商品类型折扣修改成功！");
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("类别折扣修改失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("类别折扣修改失败！请确认您拥有该类别的商品。");
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::EndChild();
        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
}

// 渲染商品列表
void UI::renderProductList(const std::vector<Product *> &products, const std::string &title)
{
    ImGui::Text("%s (%d 件商品)", title.c_str(), products.size());

    if (products.empty())
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "没有商品");
        return;
    }

    // 商品表格
    if (ImGui::BeginTable("ProductTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("名称");
        ImGui::TableSetupColumn("描述");
        ImGui::TableSetupColumn("价格");
        ImGui::TableSetupColumn("库存");
        ImGui::TableSetupColumn("商家");
        ImGui::TableHeadersRow();

        for (const auto &product : products)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", product->getName().c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::TextWrapped("%s", product->getDescription().c_str());

            ImGui::TableSetColumnIndex(2);
            if (product->getDiscountRate() > 0)
            {
                ImGui::Text("CNY%.2f (%.0f%%折)", product->getPrice(),
                            (1.0 - product->getDiscountRate()) * 100);
            }
            else
            {
                ImGui::Text("CNY%.2f", product->getPrice());
            }

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%d", product->getQuantity());

            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%s", product->getSellerUsername().c_str());
        }

        ImGui::EndTable();
    }
}

// 获取菜单结果 (兼容原代码)
int UI::getMenuResult()
{
    if (shouldClose)
        return 0; // 退出程序
    if (currentPage == 2)
        return 3; // 进入商城
    if (!currentUser)
        return 1; // 继续主循环
    return 1;     // 默认继续主循环
}

// 渲染购物车窗口
void UI::renderCartWindow()
{
    Customer *customer = dynamic_cast<Customer *>(currentUser);
    if (!customer)
        return;

    ImGui::SetNextWindowPos(ImVec2(width / 2 - 300, height / 2 - 200));
    ImGui::SetNextWindowSize(ImVec2(600, 400));

    if (ImGui::Begin("购物车", &showCartWindow, ImGuiWindowFlags_NoCollapse))
    {
        if (customer->shoppingCartItems.empty())
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "购物车为空");
        }
        else
        {
            // 购物车表格
            if (ImGui::BeginTable("CartTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
                ImGui::TableSetupColumn("商品名称");
                ImGui::TableSetupColumn("数量");
                ImGui::TableSetupColumn("单价");
                ImGui::TableSetupColumn("小计");
                ImGui::TableSetupColumn("商家");
                ImGui::TableSetupColumn("操作");
                ImGui::TableHeadersRow();

                double totalAmount = 0.0;

                std::vector<int> itemsToRemove;
                int itemIndex = 0;

                // 显示购物车中的商品
                for (auto &item : customer->shoppingCartItems)
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", item.productName.c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushID(itemIndex);
                    int quantity = item.quantity;
                    if (ImGui::InputInt("##qty", &quantity, 1, 5))
                    {
                        if (quantity > 0)
                        {
                            // 检查库存
                            Product *product = store->findProductByName(item.productName, item.sellerUsername);
                            if (product && product->getQuantity() >= quantity)
                            {
                                item.quantity = quantity;
                                customer->saveCartToFile();
                            }
                        }
                    }
                    ImGui::PopID();

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("CNY %.2f", item.priceAtAddition);

                    ImGui::TableSetColumnIndex(3);
                    double subtotal = item.priceAtAddition * item.quantity;
                    totalAmount += subtotal;
                    ImGui::Text("CNY %.2f", subtotal);

                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%s", item.sellerUsername.c_str());

                    ImGui::TableSetColumnIndex(5);
                    ImGui::PushID(1000 + itemIndex);
                    if (ImGui::Button("删除"))
                    {
                        itemsToRemove.push_back(itemIndex);
                    }
                    ImGui::PopID();

                    itemIndex++;
                }

                ImGui::EndTable();

                // 处理要删除的商品
                for (int i = itemsToRemove.size() - 1; i >= 0; i--)
                {
                    int idx = itemsToRemove[i];
                    if (idx >= 0 && idx < customer->shoppingCartItems.size())
                    {
                        customer->removeCartItem(customer->shoppingCartItems[idx].productName);
                    }
                }

                // 显示总计
                ImGui::Separator();
                ImGui::Text("总计: CNY %.2f", totalAmount);

                // 结算按钮
                if (ImGui::Button("结算", ImVec2(120, 30)))
                {
                    if (totalAmount > 0)
                    {
                        // 检查用户余额
                        if (customer->checkBalance() < totalAmount)
                        {
                            ImGui::OpenPopup("余额不足");
                        }
                        else
                        {
                            // 创建订单
                            Order order(customer->getUsername());

                            bool allItemsAvailable = true;

                            // 验证所有商品库存
                            for (auto &item : customer->shoppingCartItems)
                            {
                                Product *product = store->findProductByName(item.productName, item.sellerUsername);
                                if (!product || product->getQuantity() < item.quantity)
                                {
                                    allItemsAvailable = false;
                                    break;
                                }
                            }

                            if (!allItemsAvailable)
                            {
                                ImGui::OpenPopup("库存不足");
                            }
                            else
                            {
                                // 添加订单项
                                for (auto &item : customer->shoppingCartItems)
                                {
                                    Product *product = store->findProductByName(item.productName, item.sellerUsername);
                                    if (product)
                                    {
                                        order.addItemFromProduct(*product, item.quantity);
                                    }
                                }

                                // 提交订单
                                if (orderManager)
                                {
                                    std::shared_ptr<Order> orderPtr = orderManager->submitOrderRequest(order);
                                    if (orderPtr)
                                    {
                                        // 清空购物车
                                        customer->clearCartAndFile();

                                        // 弹出成功消息
                                        ImGui::OpenPopup("订单提交成功");
                                    }
                                }
                            }
                        }
                    }
                }

                // 弹窗
                if (ImGui::BeginPopupModal("余额不足", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("您的余额不足，需要充值。当前余额: CNY%.2f, 订单金额: CNY%.2f",
                                customer->checkBalance(), totalAmount);

                    if (ImGui::Button("确定", ImVec2(120, 0)))
                    {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                if (ImGui::BeginPopupModal("库存不足", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("部分商品库存不足，请调整购买数量。");

                    if (ImGui::Button("确定", ImVec2(120, 0)))
                    {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                if (ImGui::BeginPopupModal("订单提交成功", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("订单提交成功，请等待处理。");

                    if (ImGui::Button("确定", ImVec2(120, 0)))
                    {
                        ImGui::CloseCurrentPopup();
                        showCartWindow = false;
                    }
                    ImGui::EndPopup();
                }
            }

            ImGui::Separator();

            // 继续购物按钮
            if (ImGui::Button("继续购物", ImVec2(120, 30)))
            {
                showCartWindow = false;
            }

            ImGui::SameLine();

            // 清空购物车按钮
            if (ImGui::Button("清空购物车", ImVec2(120, 30)))
            {
                if (!customer->shoppingCartItems.empty())
                {
                    ImGui::OpenPopup("确认清空");
                }
            }

            if (ImGui::BeginPopupModal("确认清空", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("确定要清空购物车吗？");

                if (ImGui::Button("确定", ImVec2(120, 0)))
                {
                    customer->clearCartAndFile();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

                if (ImGui::Button("取消", ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }
        }

        ImGui::End();
    }
}

// 渲染订单管理窗口
void UI::renderOrderManagement()
{
    if (!currentUser || userType != 1 || !orderManager)
        return;

    Customer *customer = dynamic_cast<Customer *>(currentUser);
    if (!customer)
        return;

    // 窗口定位居中，方便用户查看
    ImGui::SetNextWindowPos(ImVec2(width / 2 - 300, height / 2 - 200), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Once);

    if (ImGui::Begin("我的订单", &showOrderWindow, ImGuiWindowFlags_NoCollapse))
    {
        static bool showPending = true;
        static bool showCompleted = true;

        ImGui::Checkbox("显示待处理订单", &showPending);
        ImGui::SameLine();
        ImGui::Checkbox("显示已完成订单", &showCompleted);

        // 订单表格高度
        float tableHeight = 250;

        // 如果点击刷新按钮，则重新加载订单数据
        if (ImGui::Button("刷新订单列表", ImVec2(150, 30)))
        {
            // 重新加载用户订单
            loadUserOrders();
        }

        ImGui::Separator();

        // 创建一个子窗口来显示订单列表
        ImGui::BeginChild("OrdersList", ImVec2(0, tableHeight), true); // 显示订单列表
        if (showPending && !pendingOrders.empty())
        {
            if (ImGui::CollapsingHeader("待处理订单", ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (int i = 0; i < pendingOrders.size(); i++)
                {
                    std::string displayName = "订单 " + pendingOrders[i]->getOrderId().substr(9) + "...";
                    if (ImGui::Selectable(displayName.c_str(), selectedOrderIndex == i))
                    {
                        selectedOrderIndex = i;
                        // 确保弹窗在根级别打开
                        ImGui::OpenPopup("##OrderDetails");
                    }
                }
            }
        }
        if (showCompleted && !completedOrders.empty())
        {
            if (ImGui::CollapsingHeader("已完成订单", ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (int i = 0; i < completedOrders.size(); i++)
                {
                    int index = i + pendingOrders.size(); // 调整索引
                    std::string displayName = "订单 " + completedOrders[i]->getOrderId().substr(9) + "...";
                    if (ImGui::Selectable(displayName.c_str(), selectedOrderIndex == index))
                    {
                        selectedOrderIndex = index;
                        // 确保弹窗在根级别打开
                        ImGui::OpenPopup("##OrderDetails");
                    }
                }
            }
        }

        // 如果没有符合条件的订单，显示提示信息
        if ((pendingOrders.empty() && completedOrders.empty()) ||
            (!showPending && !showCompleted) ||
            (pendingOrders.empty() && showPending && !showCompleted) ||
            (completedOrders.empty() && !showPending && showCompleted))
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "没有符合条件的订单");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "请点击\"刷新订单列表\"按钮加载订单");
        }

        if (ImGui::BeginPopupModal("##OrderDetails", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            std::shared_ptr<Order> order;
            if (selectedOrderIndex >= 0)
            {
                if (selectedOrderIndex < pendingOrders.size())
                {
                    order = pendingOrders[selectedOrderIndex];
                }
                else
                {
                    int completedIndex = selectedOrderIndex - pendingOrders.size();
                    if (completedIndex < completedOrders.size())
                    {
                        order = completedOrders[completedIndex];
                    }
                }
            }

            if (order)
            {
                renderOrderDetails(order);
            }
            else
            {
                ImGui::Text("订单详情加载失败");
                if (ImGui::Button("关闭", ImVec2(120, 30)))
                {
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }
        ImGui::EndChild(); // 显示订单详情弹窗

        ImGui::Separator();

        // 底部按钮 - 关闭窗口
        if (ImGui::Button("关闭窗口", ImVec2(120, 30)))
        {
            showOrderWindow = false;
        }

        ImGui::End();
    }
}

// 渲染订单详情
void UI::renderOrderDetails(std::shared_ptr<Order> order)
{
    if (!order)
        return;

    // 订单基本信息
    ImGui::Text("订单编号: %s", order->getOrderId().c_str());
    ImGui::Text("下单用户: %s", order->getCustomerUsername().c_str());
    ImGui::Text("订单状态: %s", order->getStatus().c_str());
    ImGui::Text("订单总额: CNY %.2f", order->getTotalAmount());

    char timeBuffer[64];
    time_t timestamp = order->getTimestamp();
    struct tm *timeinfo = localtime(&timestamp);
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    ImGui::Text("下单时间: %s", timeBuffer);

    ImGui::Separator(); // 订单商品列表
    ImGui::Text("订单商品:");

    if (ImGui::BeginTable("OrderItemTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("商品名称");
        ImGui::TableSetupColumn("数量");
        ImGui::TableSetupColumn("单价");
        ImGui::TableSetupColumn("商家");
        ImGui::TableHeadersRow();

        const std::vector<OrderItem> &items = order->getItems();
        if (items.empty())
        {
            // 如果没有商品，显示一条信息
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "（无商品数据）");
        }
        else
        {
            // 显示所有商品
            for (const auto &item : items)
            {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(item.productName.c_str()); // 使用TextUnformatted避免格式化问题

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%d", item.quantity);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("CNY %.2f", item.priceAtPurchase);

                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(item.sellerUsername.c_str());
            }
        }

        ImGui::EndTable();
    }

    ImGui::Separator();

    // 订单操作按钮
    if (order->getStatus() == "PENDING_CONFIRMATION" || order->getStatus() == "PENDING_IN_QUEUE")
    {
        if (ImGui::Button("取消订单", ImVec2(120, 30)))
        {
            // 添加取消订单的代码
            // TODO: 实现取消订单功能
            ImGui::OpenPopup("订单取消确认");
        }

        if (ImGui::BeginPopupModal("订单取消确认", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("确定要取消此订单吗？操作无法撤销。");

            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                // 实际取消订单
                // TODO: 实现实际取消订单功能
                ImGui::CloseCurrentPopup();
                ImGui::CloseCurrentPopup(); // 关闭订单详情弹窗
            }

            ImGui::SameLine();

            if (ImGui::Button("取消", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::SameLine();
    }

    if (ImGui::Button("关闭", ImVec2(120, 30)))
    {
        // 关闭订单详情窗口
        selectedOrderIndex = -1;
        ImGui::CloseCurrentPopup();
    }
}

// 处理订单 未完成
void UI::processOrders()
{
    if (!orderManager || !currentUser)
        return;

    // 每隔一定时间检查一次订单状态
    static float lastCheckTime = 0.0f;
    static float checkInterval = 1.0f; // 1秒检查一次

    ImGuiIO &io = ImGui::GetIO();
    float currentTime = io.DeltaTime;
    lastCheckTime += currentTime;

    if (lastCheckTime >= checkInterval)
    {
        // 重置计时器
        lastCheckTime = 0.0f;

        // 获取待处理订单数
        size_t pendingCount = orderManager->getPendingOrderCount();

        // 如果有待处理订单，显示提示
        if (pendingCount > 0 && currentUser)
        {
            if (userType == 1) // 消费者
            {
                // 更新消费者的订单列表
                // 这里可以添加代码获取消费者的订单

                // 显示订单更新通知
                // 这里可以添加代码显示订单状态更新的通知
            }
            else if (userType == 2) // 商家
            {
                // 商家可以看到自己需要处理的订单
                // 这里可以添加代码获取商家的订单
            }
        }
    }
}

// 加载用户订单
void UI::loadUserOrders()
{
    if (!orderManager || !currentUser || userType != 1)
        return;

    Customer *customer = dynamic_cast<Customer *>(currentUser);
    if (!customer)
        return;

    // 调用订单管理器获取该用户的所有订单
    orderManager->getUserOrders(customer->getUsername(), pendingOrders, completedOrders);

    // 日志输出加载的订单数量
    std::cout << "已加载用户 " << customer->getUsername()
              << " 的订单: 待处理 " << pendingOrders.size()
              << " 个, 已完成 " << completedOrders.size() << " 个" << std::endl;
}
