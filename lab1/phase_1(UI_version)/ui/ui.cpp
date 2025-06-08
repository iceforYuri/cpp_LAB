#include "ui.h"
#include "../user/user.h"
#include "../store/store.h"
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glfw.h"
#include "../imgui/backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <string>

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
    {
        // 设置用户类型
        std::string type = user->getUserType();
        if (type == "customer")
            userType = 1;
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
    store = &storeRef;

    // 主循环
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
                newUser = new Customer(regUsername, regPassword);
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
    ImGui::BeginChild("StoreControls", ImVec2(width, 50));
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
    }

    // 购买商品区域
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

        if (ImGui::Button("购买", ImVec2(100, 30)))
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
                    success = store->createGeneral(
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
