#include "client_ui.h"

// ClientUI构造函数
ClientUI::ClientUI(int width, int height)
    : width(width), height(height),
      originalWidth(width), originalHeight(height),
      isDarkTheme(true),
      primaryColor(ImVec4(0.26f, 0.59f, 0.98f, 1.00f)),
      secondaryColor(ImVec4(0.60f, 0.60f, 0.60f, 1.00f)),
      backgroundColor(ImVec4(0.06f, 0.06f, 0.06f, 1.00f))
{
    networkClient = std::make_unique<NetworkClient>();
}

// ClientUI析构函数
ClientUI::~ClientUI()
{
    // 清理ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(); // 清理GLFW
    if (this->window)
    {
        glfwDestroyWindow(this->window);
        this->window = nullptr;
    }
    glfwTerminate();
}

// 初始化UI
bool ClientUI::init()
{
    // 初始化GLFW
    if (!glfwInit())
    {
        std::cerr << "GLFW初始化失败" << std::endl;
        return false;
    } // 创建窗口
    this->window = glfwCreateWindow(width, height, "电子商城系统 - 客户端", nullptr, nullptr);
    if (!this->window)
    {
        std::cerr << "创建窗口失败" << std::endl;
        glfwTerminate();
        return false;
    }

    // 设置当前上下文
    glfwMakeContextCurrent(this->window);
    glfwSwapInterval(1); // 启用垂直同步

    // 设置窗口大小回调函数
    glfwSetWindowUserPointer(this->window, this);
    glfwSetFramebufferSizeCallback(this->window, framebufferSizeCallback); // 初始化ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // 启用键盘控制
                                                          // 加载中文字体
    ImFont *font = nullptr;

    // 尝试加载多个可能的中文字体
    const char *fontPaths[] = {
        "c:\\Windows\\Fonts\\simhei.ttf", // 黑体
        "c:\\Windows\\Fonts\\simsun.ttc", // 宋体
        "c:\\Windows\\Fonts\\msyh.ttc",   // 微软雅黑
        "c:\\Windows\\Fonts\\simkai.ttf", // 楷体
        "c:\\Windows\\Fonts\\simfang.ttf" // 仿宋
    };

    for (const char *fontPath : fontPaths)
    {
        font = io.Fonts->AddFontFromFileTTF(fontPath, 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
        if (font)
        {
            std::cout << "成功加载字体: " << fontPath << std::endl;
            break;
        }
    }

    if (!font)
    {
        std::cout << "警告: 无法加载中文字体，将使用默认字体" << std::endl;
        // 使用默认字体，但仍然设置中文字符范围
        io.Fonts->AddFontDefault();
        io.Fonts->GetGlyphRangesChineseFull();
    }

    // 设置现代化ImGui风格
    setupModernTheme();

    // 平台/渲染器后端
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    return true;
}

// 连接到服务器
bool ClientUI::connectToServer(const std::string &address, int port)
{
    if (!networkClient)
    {
        return false;
    }

    return networkClient->connect();
}

// 断开服务器连接
void ClientUI::disconnectFromServer()
{
    if (networkClient)
    {
        if (isLoggedIn)
        {
            networkClient->logout();
        }
        networkClient->disconnect();
    }
}

// 主循环
void ClientUI::mainLoop()
{
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

        // 显示购物车窗口
        if (showCartWindow && isLoggedIn && userType == Protocol::UserType::CUSTOMER)
        {
            renderCartWindow();
        }

        // 显示订单管理窗口
        if (showOrderWindow && isLoggedIn && userType == Protocol::UserType::CUSTOMER)
        {
            renderOrderWindow();
        }

        // // 显示添加商品窗口
        // if (isLoggedIn && userType == Protocol::UserType::SELLER)
        // {
        //     renderAddProductWindow();
        // } // 渲染状态消息
        // renderStatusMessages();        // 渲染确认对话框和弹窗
        if (showPurchaseConfirmDialog)
        {
            renderPurchaseConfirmDialog();
        }
        if (showDirectPurchaseConfirmDialog)
        {
            renderDirectPurchaseConfirmDialog();
        }
        if (showCartCheckoutConfirmDialog)
        {
            renderCartCheckoutConfirmDialog();
        }
        if (showAddToCartSuccessPopup)
        {
            renderAddToCartSuccessPopup();
        }
        if (showDirectPurchaseSuccessPopup)
        {
            renderDirectPurchaseSuccessPopup();
        }
        if (showPasswordChangeDialog)
        {
            renderPasswordChangeDialog();
        }
        if (showPasswordChangeSuccessDialog)
        {
            renderPasswordChangeSuccessDialog();
        }
        if (showPasswordChangeFailDialog)
        {
            renderPasswordChangeFailDialog();
        }
        if (showRechargeDialog)
        {
            renderRechargeDialog();
        }
        if (showRechargeSuccessDialog)
        {
            renderRechargeSuccessDialog();
        }
        if (showRechargeFailDialog)
        {
            renderRechargeFailDialog();
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
void ClientUI::renderMainMenu()
{
    // 创建现代化的欢迎界面
    ImVec2 windowSize = ImGui::GetWindowSize();
    float centerX = windowSize.x * 0.5f;

    // 标题区域
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 20));
    // 主标题
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::SetWindowFontScale(2.0f);
    ImGui::SetCursorPosX(centerX - ImGui::CalcTextSize("电子商城系统").x * 0.5f);
    ImGui::TextColored(this->primaryColor, "电子商城系统");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();

    // 副标题
    ImGui::SetWindowFontScale(1.2f);
    const char *subtitle = "现代化购物体验平台";
    ImGui::SetCursorPosX(centerX - ImGui::CalcTextSize(subtitle).x * 0.5f);
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), subtitle);
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    // 主要功能按钮区域
    ImVec2 buttonSize(260, 60);
    float buttonSpacing = 20;

    ImGui::SetCursorPosX(centerX - buttonSize.x * 0.5f);
    renderModernButton("注册新用户", buttonSize, true);
    if (ImGui::IsItemClicked())
    {
        currentPage = 3; // 切换到注册页面
    }

    ImGui::Spacing();
    ImGui::SetCursorPosX(centerX - buttonSize.x * 0.5f);
    renderModernButton("用户登录", buttonSize, false);
    if (ImGui::IsItemClicked())
    {
        currentPage = 4; // 切换到登录页面
        memset(usernameBuffer, 0, sizeof(usernameBuffer));
        memset(passwordBuffer, 0, sizeof(passwordBuffer));
    }

    ImGui::Spacing();
    ImGui::SetCursorPosX(centerX - buttonSize.x * 0.5f);

    // 游客模式按钮 - 不同的样式
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

    if (ImGui::Button("进入商城 (游客)", buttonSize))
    {
        currentPage = 2; // 切换到商城
        refreshProducts();
    }
    ImGui::PopStyleColor(3);

    ImGui::Spacing();
    ImGui::Spacing();

    // 退出按钮 - 放在底部
    ImGui::SetCursorPosX(centerX - 100);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));

    if (ImGui::Button("退出程序", ImVec2(200, 40)))
    {
        shouldClose = true;
    }
    ImGui::PopStyleColor(3);

    ImGui::PopStyleVar();

    // 在底部显示版本信息和主题切换
    ImGui::SameLine();
    ImGui::SetCursorPosX(windowSize.x - 120);
    if (ImGui::SmallButton(isDarkTheme ? "浅色主题" : "深色主题"))
    {
        isDarkTheme = !isDarkTheme;
        setupModernTheme();
    }
}

// 渲染用户菜单
void ClientUI::renderUserMenu()
{
    if (!isLoggedIn)
    {
        currentPage = 0; // 回到主菜单
        return;
    }

    std::string userTypeStr;
    if (userType == Protocol::UserType::CUSTOMER)
        userTypeStr = "消费者";
    else if (userType == Protocol::UserType::SELLER)
        userTypeStr = "商家";
    else if (userType == Protocol::UserType::ADMIN)
        userTypeStr = "管理员";
    else
        userTypeStr = "未知";

    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("%s菜单 - %s", userTypeStr.c_str(), currentUser.username.c_str());
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();

    // 用户信息
    ImGui::BeginChild("UserInfo", ImVec2(width * 0.4f, height - 150), true);
    ImGui::Text("用户名: %s", currentUser.username.c_str());
    ImGui::Text("用户类型: %s", userTypeStr.c_str());
    ImGui::Text("账户余额: CNY%.2f", currentUser.balance);
    ImGui::EndChild();

    // 操作区域
    ImGui::SameLine();
    ImGui::BeginChild("UserActions", ImVec2(0, height - 150), true); // 修改密码
    if (ImGui::Button("修改密码", ImVec2(100, 30)))
    {
        showPasswordChangeDialog = true;
    }

    ImGui::Separator();

    // 消费者特有 - 充值
    if (userType == Protocol::UserType::CUSTOMER)
    {
        if (ImGui::Button("账户充值", ImVec2(100, 30)))
        {
            showRechargeDialog = true;
            rechargeAmount = 0.0f;
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
        refreshProducts();
    }

    ImGui::SameLine();
    if (ImGui::Button("退出登录", ImVec2(120, 40)))
    {
        logout();
        currentPage = 0; // 回到主菜单
    }
}

// 渲染商城
void ClientUI::renderStore()
{
    ImGui::SetWindowFontScale(1.5f);
    if (isLoggedIn)
    {
        if (userType == Protocol::UserType::SELLER)
        {
            ImGui::Text("商城管理 - %s", currentUser.username.c_str());
        }
        else
        {
            ImGui::Text("商城 - %s", currentUser.username.c_str());
        }
    }
    else
    {
        ImGui::Text("商城 - 游客模式");
    }
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();

    // 根据用户类型显示不同的商城界面
    if (userType == Protocol::UserType::SELLER)
    {
        // 商家
        renderStoreSeller();
    }
    else
    {
        // 消费者或游客
        renderStoreCustomer();
    }

    // 底部按钮
    ImGui::Separator();
    if (ImGui::Button("返回", ImVec2(120, 40)))
    {
        if (isLoggedIn)
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
void ClientUI::renderUserRegister()
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
        else
        {
            Protocol::UserType type = (regUserType == 0) ? Protocol::UserType::CUSTOMER : Protocol::UserType::SELLER;
            if (networkClient && networkClient->registerUser(regUsername, regPassword, type))
            {
                // 注册成功后自动登录
                Protocol::UserData userData;
                if (networkClient->login(regUsername, regPassword, userData))
                {
                    currentUser = userData;
                    userType = userData.userType;
                    isLoggedIn = true;
                    ImGui::OpenPopup("注册成功");
                    currentPage = 1; // 直接跳转到用户菜单
                    clearMessages();
                }
                else
                {
                    ImGui::OpenPopup("登录失败");
                }
                memset(regUsername, 0, sizeof(regUsername));
                memset(regPassword, 0, sizeof(regPassword));
            }
            else
            {
                ImGui::OpenPopup("用户名已存在");
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("返回", ImVec2(120, 40)))
    {
        currentPage = 0; // 回到主菜单
    }

    // 注册相关弹窗
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
    if (ImGui::BeginPopupModal("注册成功", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("注册成功！已自动为您登录。");
        if (ImGui::Button("确定", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("登录失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("注册成功但自动登录失败，请手动登录。");
        if (ImGui::Button("确定", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
            currentPage = 4; // 跳转到登录页面
        }
        ImGui::EndPopup();
    }
}

// 渲染登录页面
void ClientUI::renderUserLogin()
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
        if (login(usernameBuffer, passwordBuffer))
        {
            clearMessages();
            currentPage = 1; // 跳转至用户菜单
        }
        else
        {
            // 根据错误类型显示不同的弹窗
            if (errorMessage == "该用户已经登录")
            {
                ImGui::OpenPopup("用户重复登录");
            }
            else if (errorMessage == "用户名或密码错误")
            {
                ImGui::OpenPopup("密码错误");
            }
            else
            {
                ImGui::OpenPopup("登录失败");
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("返回", ImVec2(120, 40)))
    {
        currentPage = 0; // 回到主菜单
    }

    // 用户重复登录弹窗
    if (ImGui::BeginPopupModal("用户重复登录", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::SetWindowFontScale(1.2f);
        ImGui::Text("登录失败");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::Separator();
        ImGui::Text("该用户已经在其他客户端登录！");
        ImGui::Text("请先在其他客户端退出登录，然后重试。");

        if (ImGui::Button("确定", ImVec2(120, 30)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // 密码错误弹窗
    if (ImGui::BeginPopupModal("密码错误", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::SetWindowFontScale(1.2f);
        ImGui::Text("登录失败");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::Separator();
        ImGui::Text("用户名或密码错误！");
        ImGui::Text("请检查您的登录信息后重试。");

        if (ImGui::Button("确定", ImVec2(120, 30)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // 通用登录失败弹窗
    if (ImGui::BeginPopupModal("登录失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::SetWindowFontScale(1.2f);
        ImGui::Text("登录失败");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::Separator();
        ImGui::Text("错误信息：%s", errorMessage.c_str());

        if (ImGui::Button("确定", ImVec2(120, 30)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

// 渲染消费者商城
void ClientUI::renderStoreCustomer()
{
    // 现代化状态栏
    renderStatusBar();

    // 现代化搜索框容器
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 15.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.13f, 0.14f, 1.0f));
    ImGui::BeginChild("SearchContainer", ImVec2(0, 120), true);

    ImGui::Spacing();
    ImGui::Text("商品搜索");
    ImGui::Spacing();

    // 现代化搜索框
    renderSearchBox();

    // 快捷操作按钮
    if (isLoggedIn && userType == Protocol::UserType::CUSTOMER)
    {
        ImGui::Spacing();

        // 居中放置按钮
        float buttonsWidth = 160.0f + 20.0f + 160.0f;
        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - buttonsWidth) * 0.5f);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.2f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.5f, 0.3f, 1.0f));
        if (ImGui::Button("查看购物车", ImVec2(160, 35)))
        {
            showCartWindow = true;
            refreshCart();
        }
        ImGui::PopStyleColor(2);

        ImGui::SameLine(0, 20.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.9f, 1.0f));
        if (ImGui::Button("查看订单", ImVec2(160, 35)))
        {
            showOrderWindow = true;
            refreshOrders();
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    ImGui::Spacing(); // 商品列表
    if (strlen(searchBuffer) > 0)
    {
        // 如果有搜索关键词，显示搜索结果
        if (!searchResults.empty())
        {
            renderProductList(searchResults, "搜索结果");
        }
        else
        {
            ImGui::Text("没有找到符合条件的商品");
        }
    }
    else
    {
        // 没有搜索关键词，显示所有商品
        renderProductList(allProducts, "所有商品");
    }

    // 购买商品区域
    if (isLoggedIn && userType == Protocol::UserType::CUSTOMER)
    {
        ImGui::Separator();
        ImGui::Text("购买商品");

        ImGui::SetNextItemWidth(250);
        ImGui::InputText("商品名称", buyProductName, IM_ARRAYSIZE(buyProductName));
        ImGui::SetNextItemWidth(150);
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
                // 查找商品
                Protocol::ProductData *targetProduct = nullptr;
                for (auto &product : allProducts)
                {
                    if (product.name == buyProductName)
                    {
                        targetProduct = &product;
                        break;
                    }
                }

                if (!targetProduct)
                {
                    ImGui::OpenPopup("商品不存在");
                }
                else if (targetProduct->quantity < buyQuantity)
                {
                    ImGui::OpenPopup("库存不足");
                }
                else
                {
                    // 首先锁定库存
                    if (networkClient->lockInventory(targetProduct->id, buyQuantity))
                    {
                        // 库存锁定成功，设置确认对话框参数并显示
                        productToPurchase = *targetProduct;
                        quantityToPurchase = buyQuantity;
                        inventoryLocked = true; // 标记库存已锁定
                        std::cout << "库存锁定成功，准备购买商品: " << productToPurchase.name << std::endl;
                        showDirectPurchaseConfirmDialog = true;

                        // 清空表单数据
                        memset(buyProductName, 0, sizeof(buyProductName));
                        buyQuantity = 1;
                    }
                    else
                    {
                        // 库存锁定失败
                        ImGui::OpenPopup("库存锁定失败");
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
                // 查找商品
                Protocol::ProductData *targetProduct = nullptr;
                for (auto &product : allProducts)
                {
                    if (product.name == buyProductName)
                    {
                        targetProduct = &product;
                        break;
                    }
                }

                if (!targetProduct)
                {
                    ImGui::OpenPopup("商品不存在");
                }
                else if (targetProduct->quantity < buyQuantity)
                {
                    ImGui::OpenPopup("库存不足");
                }
                else
                {
                    if (networkClient->addToCart(targetProduct->id, buyQuantity))
                    {
                        // ImGui::OpenPopup("加入购物车成功");
                        addToCartSuccessMessage = "商品 \"" + targetProduct->name + "\" 已成功加入购物车！";
                        showAddToCartSuccessPopup = true;
                        memset(buyProductName, 0, sizeof(buyProductName));
                        buyQuantity = 1;
                        refreshCart();
                    }
                    else
                    {
                        ImGui::OpenPopup("加入购物车失败");
                    }
                }
            }
        }
    }

    // 购买相关弹窗
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
        ImGui::Text("商品 \"%s\" 库存不足！", buyProductName);
        ImGui::Text("您要购买 %d 件，但库存只有 %d 件。", buyQuantity, 0); // 需要找到库存数量
        if (ImGui::Button("确定", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal("余额不足", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("您的余额不足以完成此次购买！");
        ImGui::Text("请先充值后再试。");
        if (ImGui::Button("确定", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("库存锁定失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("库存锁定失败！");
        ImGui::Text("商品可能库存不足或已售完，请稍后重试。");
        if (ImGui::Button("确定", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // if (ImGui::BeginPopupModal("购买成功", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    // {
    //     ImGui::Text("购买成功！");
    //     if (ImGui::Button("确定", ImVec2(120, 0)))
    //     {
    //         ImGui::CloseCurrentPopup();
    //     }
    //     ImGui::EndPopup();
    // }

    // if (ImGui::BeginPopupModal("加入购物车成功", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    // {
    //     ImGui::Text("%s", addToCartSuccessMessage.c_str());
    //     ImGui::Separator();

    //     if (ImGui::Button("查看购物车", ImVec2(120, 30)))
    //     {
    //         showCartWindow = true;
    //         showAddToCartSuccessPopup = false;
    //         ImGui::CloseCurrentPopup();
    //     }
    //     ImGui::SameLine();
    //     if (ImGui::Button("继续购物", ImVec2(120, 30)))
    //     {
    //         showAddToCartSuccessPopup = false;
    //         ImGui::CloseCurrentPopup();
    //     }
    //     ImGui::EndPopup();
    // }

    if (ImGui::BeginPopupModal("加入购物车失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("加入购物车失败，请重试！");
        if (ImGui::Button("确定", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("购买失败-cart", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("购买失败：购物车操作异常！");
        if (ImGui::Button("确定", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("购买失败-add", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("购买失败：无法加入购物车！");
        if (ImGui::Button("确定", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

// 渲染商家商城
void ClientUI::renderStoreSeller()
{
    if (!isLoggedIn || userType != Protocol::UserType::SELLER)
        return;

    // 显示选项卡
    ImGui::BeginTabBar("SellerTabs");

    if (ImGui::BeginTabItem("我的商品"))
    {
        ImGui::BeginChild("MyProducts", ImVec2(0, height - 200));
        std::vector<Protocol::ProductData> sellerProducts;
        for (const auto &product : allProducts)
        {
            if (product.sellerUsername == currentUser.username)
            {
                sellerProducts.push_back(product);
            }
        }
        renderProductList(sellerProducts, "我的商品");
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
                Protocol::ProductData newProduct;
                newProduct.name = newProductName;
                newProduct.description = newProductDesc;
                newProduct.price = newProductPrice;
                newProduct.originalPrice = newProductPrice; // 设置原价
                newProduct.quantity = newProductQuantity;
                newProduct.discountRate = 0.0; // 初始折扣为0
                newProduct.sellerUsername = currentUser.username;
                newProduct.id = newProductName; // 使用商品名作为ID

                // 根据类型设置type和属性
                if (newProductType == 0)
                { // 图书
                    newProduct.type = "Book";
                    newProduct.attributes["author"] = newProductAttr1;
                    newProduct.attributes["isbn"] = newProductAttr2;
                }
                else if (newProductType == 1)
                { // 服装
                    newProduct.type = "Clothing";
                    newProduct.attributes["size"] = newProductAttr1;
                    newProduct.attributes["color"] = newProductAttr2;
                }
                else if (newProductType == 2)
                { // 食品
                    newProduct.type = "Food";
                    newProduct.attributes["expirationDate"] = newProductAttr1;
                }
                else
                { // 其他（自定义商品）
                    newProduct.type = "Generic";
                    newProduct.attributes["categoryTag"] = newProductAttr1;
                }

                if (addProduct(newProduct))
                {
                    ImGui::OpenPopup("商品添加成功");
                    memset(newProductName, 0, sizeof(newProductName));
                    memset(newProductDesc, 0, sizeof(newProductDesc));
                    memset(newProductAttr1, 0, sizeof(newProductAttr1));
                    memset(newProductAttr2, 0, sizeof(newProductAttr2));
                    newProductPrice = 0.0f;
                    newProductQuantity = 0;
                    refreshProducts();
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

        if (ImGui::BeginPopupModal("商品添加失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("商品添加失败！请填写商品名称。");
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
                if (networkClient && networkClient->manageProductPrice(priceProductName, newPrice))
                {
                    ImGui::OpenPopup("价格修改成功");
                    memset(priceProductName, 0, sizeof(priceProductName));
                    newPrice = 0.0f;
                    refreshProducts(); // 刷新商品列表
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
                if (networkClient && networkClient->manageProductQuantity(qtyProductName, newQty))
                {
                    ImGui::OpenPopup("库存修改成功");
                    memset(qtyProductName, 0, sizeof(qtyProductName));
                    newQty = 0;
                    refreshProducts(); // 刷新商品列表
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
                if (networkClient && networkClient->manageProductDiscount(discountProductName, newDiscount / 100.0))
                {
                    ImGui::OpenPopup("折扣修改成功");
                    memset(discountProductName, 0, sizeof(discountProductName));
                    newDiscount = 0.0f;
                    refreshProducts(); // 刷新商品列表
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

            if (networkClient && networkClient->applyCategoryDiscount(category, categoryDiscount / 100.0))
            {
                ImGui::OpenPopup("类别折扣修改成功");
                categoryDiscount = 0.0f;
                if (selectedCategory == 3)
                {
                    memset(categoryName, 0, sizeof(categoryName));
                }
                refreshProducts(); // 刷新商品列表
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

        if (ImGui::BeginPopupModal("价格修改失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("价格修改失败！请检查商品名称和权限。");
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("库存修改失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("库存修改失败！请检查商品名称和权限。");
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("折扣修改失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("折扣修改失败！请检查商品名称和权限。");
            if (ImGui::Button("确定", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("类别折扣修改失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("类别折扣修改失败！请检查分类名称和权限。");
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
void ClientUI::renderProductList(const std::vector<Protocol::ProductData> &products, const std::string &title)
{
    ImGui::Text("%s (%d 件商品)", title.c_str(), (int)products.size());

    if (products.empty())
    {
        ImGui::Text("暂无商品");
        return;
    }

    // 商品表格
    if (ImGui::BeginTable("ProductTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("商品名称", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("价格", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("库存", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("商家", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("类型", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableHeadersRow();

        for (const auto &product : products)
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", product.name.c_str());
            ImGui::TableSetColumnIndex(1);
            // 显示价格信息，包含折扣
            if (product.discountRate > 0.0)
            {
                // 有折扣，显示原价和现价
                // ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "CNY%.2f", product.originalPrice);
                // ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "CNY%.2f", product.price);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "(%.0f%%折)", (1 - product.discountRate) * 100);
            }
            else
            {
                // 无折扣，正常显示价格
                ImGui::Text("CNY%.2f", product.price);
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%d", product.quantity);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", product.sellerUsername.c_str());

            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%s", product.type.c_str());
            ImGui::TableSetColumnIndex(5);
            if (isLoggedIn && userType == Protocol::UserType::CUSTOMER)
            {
                std::string buttonId = "加入购物车##" + product.id;
                // 检查库存状态，如果没有库存则禁用按钮
                bool hasStock = product.quantity > 0;

                if (!hasStock)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                }

                if (ImGui::Button(hasStock ? buttonId.c_str() : ("缺货##" + product.id).c_str()) && hasStock)
                {
                    addProductToCart(product);
                }

                if (!hasStock)
                {
                    ImGui::PopStyleColor(3);
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("商品缺货，暂时无法购买");
                    }
                }
            }
        }

        ImGui::EndTable();
    }
}

// 渲染购物车窗口
void ClientUI::renderCartWindow()
{
    if (!isLoggedIn || userType != Protocol::UserType::CUSTOMER)
        return;

    ImGui::SetNextWindowPos(ImVec2(width / 2 - 400, height / 2 - 250));
    ImGui::SetNextWindowSize(ImVec2(800, 500));

    if (ImGui::Begin("购物车", &showCartWindow, ImGuiWindowFlags_NoCollapse))
    {
        if (cartItems.empty())
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "购物车为空");
            ImGui::Spacing();
            ImGui::Text("去商城添加您喜欢的商品吧！");
        }
        else
        {
            if (ImGui::BeginTable("CartTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
            {
                ImGui::TableSetupColumn("商品名称", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableSetupColumn("数量", ImGuiTableColumnFlags_WidthFixed, 150.0f); // 增加数量列宽度从80到100
                ImGui::TableSetupColumn("单价", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("小计", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("商家", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("操作", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableHeadersRow();

                double totalAmount = 0.0;
                std::vector<int> itemsToRemove;

                for (size_t i = 0; i < cartItems.size(); ++i)
                {
                    const auto &item = cartItems[i];
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", item.productName.c_str());
                    ImGui::TableSetColumnIndex(1);
                    // 可编辑的数量输入框 - 增加宽度使其更美观
                    ImGui::PushID(static_cast<int>(i));
                    int quantity = item.quantity;
                    ImGui::SetNextItemWidth(120.0f);                // 增加宽度从60到75
                    if (ImGui::InputInt("##qty", &quantity, 1, 10)) // 修复步长参数：slow_step=1, fast_step=10
                    {
                        if (quantity > 0)
                        {
                            // 检查库存
                            Protocol::ProductData *product = nullptr;
                            for (auto &p : allProducts)
                            {
                                if (p.id == item.productId)
                                {
                                    product = &p;
                                    break;
                                }
                            }
                            if (product && product->quantity >= quantity)
                            {
                                updateCartQuantity(item.productId, quantity);
                                // 立即更新本地购物车以反映更改
                                refreshCart();
                            }
                            else if (product)
                            {
                                ImGui::OpenPopup("库存不足");
                            }
                            else
                            {
                                ImGui::OpenPopup("商品不存在");
                            }
                        }
                        else if (quantity == 0)
                        {
                            // 数量为0时移除商品
                            itemsToRemove.push_back(static_cast<int>(i));
                        }
                    }
                    ImGui::PopID();
                    ImGui::TableSetColumnIndex(2);
                    // 显示当前的折扣后价格，而不是加入时的价格
                    double currentPrice = item.priceAtAddition; // 默认使用加入时价格
                    bool priceChanged = false;

                    // 查找当前商品的实际价格
                    for (const auto &product : allProducts)
                    {
                        if (product.id == item.productId)
                        {
                            currentPrice = product.price; // 使用当前折扣后价格
                            priceChanged = (currentPrice != item.priceAtAddition);
                            break;
                        }
                    }

                    if (priceChanged)
                    {
                        // 价格有变化，显示当前价格并提示
                        ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "CNY%.2f", currentPrice);
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("原价格: CNY%.2f", item.priceAtAddition);
                        }
                    }
                    else
                    {
                        ImGui::Text("CNY%.2f", currentPrice);
                    }

                    ImGui::TableSetColumnIndex(3);
                    double subtotal = currentPrice * item.quantity;
                    ImGui::Text("CNY%.2f", subtotal);
                    totalAmount += subtotal;

                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%s", item.sellerUsername.c_str());

                    ImGui::TableSetColumnIndex(5);
                    std::string removeButtonId = "移除##" + item.productId;
                    if (ImGui::Button(removeButtonId.c_str(), ImVec2(50, 25)))
                    {
                        itemsToRemove.push_back(static_cast<int>(i));
                    }
                }

                ImGui::EndTable();

                // 处理要删除的商品（从后往前删除，避免索引问题）
                for (int i = itemsToRemove.size() - 1; i >= 0; i--)
                {
                    int idx = itemsToRemove[i];
                    if (idx >= 0 && idx < cartItems.size())
                    {
                        removeFromCart(cartItems[idx].productId);
                    }
                }

                ImGui::Separator();
                ImGui::Text("总计: CNY%.2f", totalAmount);

                // 结算和操作按钮
                if (ImGui::Button("结算", ImVec2(100, 30)))
                {
                    if (totalAmount > 0)
                    {
                        // 检查用户余额
                        if (currentUser.balance < totalAmount)
                        {
                            ImGui::OpenPopup("余额不足");
                        }
                        else
                        {
                            showCartCheckoutConfirmDialog = true;
                        }
                    }
                }
                ImGui::SameLine();

                if (ImGui::Button("继续购物", ImVec2(100, 30)))
                {
                    showCartWindow = false;
                }
                ImGui::SameLine();

                if (ImGui::Button("清空购物车", ImVec2(120, 30)))
                {
                    if (!cartItems.empty())
                    {
                        ImGui::OpenPopup("确认清空");
                    }
                }

                // 余额不足弹窗
                if (ImGui::BeginPopupModal("余额不足", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("您的余额不足，需要充值。");
                    ImGui::Text("当前余额: CNY%.2f", currentUser.balance);
                    ImGui::Text("订单金额: CNY%.2f", totalAmount);
                    ImGui::Text("还需充值: CNY%.2f", totalAmount - currentUser.balance);

                    if (ImGui::Button("去充值", ImVec2(120, 0)))
                    {
                        showRechargeDialog = true;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("取消", ImVec2(120, 0)))
                    {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                // 库存不足弹窗
                if (ImGui::BeginPopupModal("库存不足", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("商品库存不足，请调整购买数量。");
                    if (ImGui::Button("确定", ImVec2(120, 0)))
                    {
                        ImGui::CloseCurrentPopup();
                        refreshCart(); // 刷新购物车显示正确数量
                    }
                    ImGui::EndPopup();
                }

                // 商品不存在弹窗
                if (ImGui::BeginPopupModal("商品不存在", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("商品可能已下架，请刷新购物车。");
                    if (ImGui::Button("确定", ImVec2(120, 0)))
                    {
                        ImGui::CloseCurrentPopup();
                        refreshCart();
                    }
                    ImGui::EndPopup();
                } // 确认清空购物车弹窗
                if (ImGui::BeginPopupModal("确认清空", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("确定要清空购物车吗？");
                    ImGui::Text("这将删除所有已添加的商品。");

                    if (ImGui::Button("确定", ImVec2(120, 0)))
                    {
                        if (networkClient && networkClient->clearCart())
                        {
                            cartItems.clear();
                            setStatus("购物车已清空");
                        }
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("取消", ImVec2(120, 0)))
                    {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                // 加入购物车失败弹窗（用于产品列表中的加入购物车按钮）
                if (ImGui::BeginPopupModal("加入购物车失败", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("加入购物车失败，请重试！");
                    ImGui::Text("可能是网络错误或商品已下架。");

                    if (ImGui::Button("确定", ImVec2(120, 0)))
                    {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
        }
    }
    ImGui::End();
}

// 渲染订单窗口
void ClientUI::renderOrderWindow()
{
    if (!isLoggedIn || userType != Protocol::UserType::CUSTOMER)
        return;

    // 窗口定位居中，方便用户查看
    ImGui::SetNextWindowPos(ImVec2(width / 2 - 300, height / 2 - 200), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_Once);

    if (ImGui::Begin("我的订单", &showOrderWindow, ImGuiWindowFlags_NoCollapse))
    {
        static bool showPending = true;
        static bool showCompleted = true;
        static int selectedOrderIndex = -1;

        // 过滤选项
        ImGui::Checkbox("显示待处理订单", &showPending);
        ImGui::SameLine();
        ImGui::Checkbox("显示已完成订单", &showCompleted);

        // 刷新按钮
        if (ImGui::Button("刷新订单列表", ImVec2(150, 30)))
        {
            refreshUserOrders();
        }

        ImGui::Separator();

        // 创建子窗口显示订单列表
        ImGui::BeginChild("OrdersList", ImVec2(0, 250), true);

        if (userOrders.empty())
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "暂无订单");
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "请点击\"刷新订单列表\"按钮加载订单");
        }
        else
        {
            // 分离待处理和已完成订单
            std::vector<Protocol::OrderData> pendingOrders, completedOrders;
            for (const auto &order : userOrders)
            {
                if (order.status == Protocol::OrderStatus::PENDING)
                {
                    pendingOrders.push_back(order);
                }
                else
                {
                    completedOrders.push_back(order);
                }
            }

            // 显示待处理订单
            if (showPending && !pendingOrders.empty())
            {
                if (ImGui::CollapsingHeader("待处理订单", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (int i = 0; i < pendingOrders.size(); i++)
                    {
                        // 修复金额显示格式
                        std::ostringstream amountStream;
                        amountStream << std::fixed << std::setprecision(2) << pendingOrders[i].totalAmount;

                        std::string orderIdPart = pendingOrders[i].orderId.length() >= 18 ? pendingOrders[i].orderId.substr(4, 14) : pendingOrders[i].orderId;
                        std::string displayName = "订单 " + orderIdPart + "... (CNY" + amountStream.str() + ")";
                        std::string selectableId = displayName + "##pending_" + std::to_string(i);
                        if (ImGui::Selectable(displayName.c_str(), selectedOrderIndex == i))
                        {
                            selectedOrderIndex = i;
                            selectedOrder = pendingOrders[i];
                            ImGui::OpenPopup("订单详情");
                        }
                    }
                }
            }

            // 显示已完成订单
            if (showCompleted && !completedOrders.empty())
            {
                if (ImGui::CollapsingHeader("已完成订单", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (int i = 0; i < completedOrders.size(); i++)
                    {
                        int index = i + pendingOrders.size();
                        std::string displayName = "订单 " + completedOrders[i].orderId.substr(0, 18) + "... (CNY" +
                                                  std::to_string(completedOrders[i].totalAmount).substr(0, 6) + ")";
                        if (ImGui::Selectable(displayName.c_str(), selectedOrderIndex == index))
                        {
                            selectedOrderIndex = index;
                            selectedOrder = completedOrders[i];
                            ImGui::OpenPopup("订单详情");
                        }
                    }
                }
            }
        }

        // 订单详情弹窗
        if (ImGui::BeginPopupModal("订单详情", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            renderOrderDetails(selectedOrder);
            ImGui::EndPopup();
        }

        ImGui::EndChild();

        ImGui::Separator();

        // 底部按钮
        if (ImGui::Button("关闭窗口", ImVec2(120, 30)))
        {
            showOrderWindow = false;
        }
    }
    ImGui::End();
}

// 渲染状态消息
void ClientUI::renderStatusMessages()
{
    if (!statusMessage.empty())
    {
        ImGui::SetNextWindowPos(ImVec2(width / 2 - 150, 50));
        ImGui::SetNextWindowBgAlpha(0.8f);
        if (ImGui::Begin("状态消息", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", statusMessage.c_str());
        }
        ImGui::End();
    }

    if (showErrorMessage && !errorMessage.empty())
    {
        ImGui::SetNextWindowPos(ImVec2(width / 2 - 150, 50));
        ImGui::SetNextWindowBgAlpha(0.8f);
        if (ImGui::Begin("错误消息", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", errorMessage.c_str());
            if (ImGui::Button("确定"))
            {
                showErrorMessage = false;
                errorMessage.clear();
            }
        }
        ImGui::End();
    }
}

// 渲染订单详情
void ClientUI::renderOrderDetails(const Protocol::OrderData &order)
{
    // 订单基本信息
    ImGui::Text("订单编号: %s", order.orderId.c_str());
    ImGui::Text("订单总额: CNY %.2f", order.totalAmount);

    std::string statusStr;
    switch (order.status)
    {
    case Protocol::OrderStatus::PENDING:
        statusStr = "待处理";
        break;
    case Protocol::OrderStatus::COMPLETED:
        statusStr = "已完成";
        break;
    case Protocol::OrderStatus::CANCELLED_STOCK:
        statusStr = "库存不足取消";
        break;
    case Protocol::OrderStatus::CANCELLED_FUNDS:
        statusStr = "余额不足取消";
        break;
    case Protocol::OrderStatus::CANCELLED_USER:
        statusStr = "用户取消";
        break;
    default:
        statusStr = "未知";
        break;
    }
    ImGui::Text("订单状态: %s", statusStr.c_str());

    // 时间格式化显示
    if (order.timestamp > 0)
    {
        time_t timestamp = static_cast<time_t>(order.timestamp);
        char timeBuffer[64];
        struct tm *timeInfo = localtime(&timestamp);
        strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", timeInfo);
        ImGui::Text("下单时间: %s", timeBuffer);
    }
    else
    {
        ImGui::Text("下单时间: 未知");
    }

    ImGui::Separator();
    ImGui::Text("订单商品:");

    // 订单商品列表
    if (ImGui::BeginTable("OrderItemTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("商品名称");
        ImGui::TableSetupColumn("数量");
        ImGui::TableSetupColumn("单价");
        ImGui::TableSetupColumn("商家");
        ImGui::TableHeadersRow();

        if (order.items.empty())
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "（无商品数据）");
        }
        else
        {
            for (const auto &item : order.items)
            {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(item.productName.c_str());

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%d", item.quantity);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("CNY %.2f", item.priceAtAddition);

                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(item.sellerUsername.c_str());
            }
        }

        ImGui::EndTable();
    }

    ImGui::Separator();

    // 订单操作按钮
    if (order.status == Protocol::OrderStatus::PENDING)
    {
        if (ImGui::Button("取消订单", ImVec2(120, 30)))
        {
            ImGui::OpenPopup("订单取消确认");
        }

        if (ImGui::BeginPopupModal("订单取消确认", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("确定要取消此订单吗？操作无法撤销。");

            if (ImGui::Button("确认取消", ImVec2(120, 30)))
            {
                // TODO: 实现取消订单功能
                if (networkClient)
                {
                    // networkClient->cancelOrder(order.orderId);
                }
                ImGui::CloseCurrentPopup();
                ImGui::CloseCurrentPopup(); // 关闭订单详情弹窗
            }
            ImGui::SameLine();
            if (ImGui::Button("取消", ImVec2(120, 30)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::SameLine();
    }

    if (ImGui::Button("关闭", ImVec2(120, 30)))
    {
        ImGui::CloseCurrentPopup();
    }
}

// 刷新用户订单
void ClientUI::refreshUserOrders()
{
    if (!networkClient || !isLoggedIn)
    {
        return;
    }

    std::vector<Protocol::OrderData> orders;
    if (networkClient->getUserOrders(orders))
    {
        userOrders = orders;
        setStatus("订单列表已刷新");
    }
    else
    {
        setError("获取订单列表失败");
    }
}

// 业务逻辑方法实现

bool ClientUI::login(const std::string &username, const std::string &password)
{
    if (!networkClient)
        return false;

    Protocol::UserData userData;
    std::string errorMessage;
    if (networkClient->loginWithError(username, password, userData, errorMessage))
    {
        currentUser = userData;
        userType = userData.userType;
        isLoggedIn = true;
        return true;
    }
    else
    {
        // 设置具体的错误信息
        setError(errorMessage);
        return false;
    }
}

bool ClientUI::registerUser(const std::string &username, const std::string &password, Protocol::UserType userType)
{
    if (!networkClient)
        return false;

    return networkClient->registerUser(username, password, userType);
}

void ClientUI::logout()
{
    if (networkClient && isLoggedIn)
    {
        networkClient->logout();
    }
    isLoggedIn = false;
    userType = Protocol::UserType::CUSTOMER;
    currentUser = Protocol::UserData();
    allProducts.clear();
    searchResults.clear();
    cartItems.clear();
    userOrders.clear();
}

void ClientUI::refreshProducts()
{
    if (!networkClient)
        return;

    std::cout << "刷新产品列表前，当前产品数量: " << allProducts.size() << std::endl;
    allProducts.clear();
    if (networkClient->getAllProducts(allProducts))
    {
        std::cout << "产品列表刷新成功，新产品数量: " << allProducts.size() << std::endl;

        // 调试：显示前几个产品的库存信息
        for (size_t i = 0; i < std::min((size_t)3, allProducts.size()); ++i)
        {
            const auto &prod = allProducts[i];
            std::cout << "产品 \"" << prod.name << "\" 库存: " << prod.quantity << std::endl;
        }
    }
    else
    {
        std::cout << "产品列表刷新失败!" << std::endl;
    }
}

void ClientUI::refreshCart()
{
    if (!networkClient || !isLoggedIn)
        return;

    cartItems.clear();
    networkClient->getCart(cartItems);
}

void ClientUI::refreshOrders()
{
    if (!networkClient || !isLoggedIn)
        return;

    userOrders.clear();
    networkClient->getUserOrders(userOrders);
}

void ClientUI::performSearch()
{
    if (!networkClient)
        return;

    searchResults.clear();
    if (strlen(searchBuffer) > 0)
    {
        networkClient->searchProducts(searchBuffer, searchResults);
    }
}

void ClientUI::addProductToCart(const Protocol::ProductData &product)
{
    if (!networkClient || !isLoggedIn)
        return;

    if (networkClient->addToCart(product.id, 1))
    {
        // 显示成功弹窗
        addToCartSuccessMessage = "商品 \"" + product.name + "\" 已成功加入购物车！";
        showAddToCartSuccessPopup = true;
        refreshCart();
    }
    else
    {
        ImGui::OpenPopup("加入购物车失败");
    }
}

void ClientUI::updateCartQuantity(const std::string &productId, int newQuantity)
{
    if (!networkClient || !isLoggedIn)
        return;
    if (networkClient->updateCartItem(productId, newQuantity))
    {
        refreshCart();
    }
    else
    {
        setError("更新购物车数量失败，请重试");
        refreshCart(); // 刷新以显示正确的数量
    }
}

void ClientUI::removeFromCart(const std::string &productId)
{
    if (!networkClient || !isLoggedIn)
        return;

    if (networkClient->removeFromCart(productId))
    {
        setStatus("商品已从购物车移除！");
        refreshCart();
    }
}

void ClientUI::checkout()
{
    if (!networkClient || !isLoggedIn || cartItems.empty())
        return;

    std::string orderId;
    if (networkClient->createOrder(cartItems, orderId))
    {
        setStatus("订单创建成功！订单号: %s", orderId.c_str());
        cartItems.clear();
        refreshOrders();
    }
    else
    {
        setError("订单创建失败！");
    }
}

void ClientUI::checkoutWithInventoryLock()
{
    if (!networkClient || !isLoggedIn || cartItems.empty())
    {
        setError("结账失败：未登录或购物车为空");
        return;
    }

    // 第一步：尝试锁定所有购物车商品的库存
    if (!networkClient->lockCartInventory(cartItems))
    {
        setError("库存锁定失败！部分商品可能库存不足或已售完");
        return;
    }

    // 第二步：创建订单
    std::string orderId;
    bool orderCreated = networkClient->createOrder(cartItems, orderId);

    if (orderCreated)
    {
        // 订单创建成功 - 库存已在服务端正确处理
        setStatus("订单创建成功！订单号: %s", orderId.c_str());
        cartItems.clear();
        refreshOrders();
        refreshCart(); // 刷新购物车显示

        // 更新用户余额信息
        Protocol::UserData updatedUser;
        if (networkClient->getUserInfo(updatedUser))
        {
            currentUser = updatedUser;
        }
    }
    else
    {
        // 订单创建失败 - 解锁库存
        if (!networkClient->unlockCartInventory(cartItems))
        {
            setError("警告：订单创建失败且库存解锁失败！请联系客服处理");
        }
        else
        {
            setError("订单创建失败！可能是余额不足或其他原因，库存已释放");
        }
    }
}

bool ClientUI::addProduct(const Protocol::ProductData &product)
{
    if (!networkClient)
        return false;

    return networkClient->addProduct(product);
}

void ClientUI::addToCartByName(const std::string &productName, int quantity)
{
    // 根据商品名称找到商品ID
    for (const auto &product : allProducts)
    {
        if (product.name == productName)
        {
            if (networkClient->addToCart(product.id, quantity))
            {
                setStatus("商品已加入购物车！");
                refreshCart();
            }
            else
            {
                setError("加入购物车失败！");
            }
            return;
        }
    }
    setError("找不到商品: %s", productName.c_str());
}

void ClientUI::purchaseProduct(const std::string &productName, int quantity)
{
    // 直接购买逻辑 - 库存已在确认对话框显示前锁定
    for (const auto &product : allProducts)
    {
        if (product.name == productName)
        {
            // 库存已锁定，直接创建订单
            std::string orderId;
            if (networkClient->createDirectOrder(product.id, quantity, orderId))
            {
                // 购买成功，重置库存锁定状态
                inventoryLocked = false;

                // 显示购买成功弹窗
                directPurchaseSuccessMessage = "购买成功！\n\n商品：" + product.name +
                                               "\n数量：" + std::to_string(quantity) +
                                               "\n总价：¥" + std::to_string(product.price * quantity) +
                                               "\n订单ID：" + orderId;

                showDirectPurchaseSuccessPopup = true;

                // 立即刷新产品信息以显示最新库存
                refreshProducts();

                // 更新用户余额信息
                Protocol::UserData updatedUser;
                if (networkClient->getUserInfo(updatedUser))
                {
                    currentUser = updatedUser;
                }

                std::cout << "直接购买成功，正在刷新商品信息..." << std::endl;
            }
            else
            {
                // 订单创建失败，解锁库存
                if (inventoryLocked && !networkClient->unlockInventory(product.id, quantity))
                {
                    setError("警告：购买失败且库存解锁失败！请联系客服处理");
                }
                else
                {
                    inventoryLocked = false;
                    setError("直接购买失败！可能是余额不足或其他原因，库存已释放");
                }
            }
            return;
        }
    }
    setError("找不到商品: %s", productName.c_str());
}

// 网络操作辅助方法
void ClientUI::setStatus(const std::string &message)
{
    statusMessage = message;
    showErrorMessage = false;
    errorMessage.clear();
}

void ClientUI::setError(const std::string &error)
{
    errorMessage = error;
    showErrorMessage = true;
    statusMessage.clear();
}

void ClientUI::setStatus(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    statusMessage = std::string(buffer);
    showErrorMessage = false;
    errorMessage.clear();
}

void ClientUI::setError(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    errorMessage = std::string(buffer);
    showErrorMessage = true;
    statusMessage.clear();
}

void ClientUI::clearMessages()
{
    statusMessage.clear();
    errorMessage.clear();
    showErrorMessage = false;
}

// 设置现代化主题
void ClientUI::setupModernTheme()
{
    ImGuiStyle &style = ImGui::GetStyle();

    // 设置圆角
    style.WindowRounding = 10.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;

    // 设置间距
    style.WindowPadding = ImVec2(15, 15);
    style.FramePadding = ImVec2(12, 8);
    style.ItemSpacing = ImVec2(12, 8);
    style.ItemInnerSpacing = ImVec2(8, 6);
    style.TouchExtraPadding = ImVec2(0, 0);
    style.IndentSpacing = 25;
    style.ScrollbarSize = 15;
    style.GrabMinSize = 10;

    // 设置边框
    style.WindowBorderSize = 1;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 0;
    style.TabBorderSize = 0;

    if (isDarkTheme)
    {
        setupDarkTheme();
    }
    else
    {
        setupLightTheme();
    }
}

void ClientUI::setupDarkTheme()
{
    ImVec4 *colors = ImGui::GetStyle().Colors;

    // 主要颜色
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.16f, 0.17f, 0.18f, 0.94f);

    // 边框
    colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // 框架背景
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 0.67f);

    // 标题栏
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);

    // 滚动条
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);

    // 按钮
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);

    // 选择
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

    // Tab
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
}

void ClientUI::setupLightTheme()
{
    ImVec4 *colors = ImGui::GetStyle().Colors;

    // 主要颜色
    colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);

    // 边框
    colors[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.70f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // 框架背景
    colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.80f, 0.80f, 0.80f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 0.67f);

    // 按钮
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
}

// 渲染现代化按钮
bool ClientUI::renderModernButton(const char *label, ImVec2 size, bool primary)
{
    if (primary)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, this->primaryColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(this->primaryColor.x * 1.2f, this->primaryColor.y * 1.2f, this->primaryColor.z * 1.2f, this->primaryColor.w));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(this->primaryColor.x * 0.8f, this->primaryColor.y * 0.8f, this->primaryColor.z * 0.8f, this->primaryColor.w));
    }

    bool pressed = ImGui::Button(label, size);

    if (primary)
    {
        ImGui::PopStyleColor(3);
    }

    return pressed;
}

// 渲染卡片
void ClientUI::renderCard(const char *title, const char *content, bool collapsible)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.16f, 0.17f, 0.18f, 1.00f));

    if (collapsible)
    {
        if (ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::BeginChild(title, ImVec2(0, 0), true);
            ImGui::Text("%s", content);
            ImGui::EndChild();
        }
    }
    else
    {
        ImGui::Text("%s", title);
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::BeginChild(title, ImVec2(0, 0), true);
        ImGui::Text("%s", content);
        ImGui::EndChild();
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

// 渲染现代化搜索框
void ClientUI::renderSearchBox()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(15.0f, 10.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.21f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.25f, 0.26f, 0.27f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    // 搜索图标和输入框

    ImGui::SetNextItemWidth(-150.0f); // 留出按钮空间
    bool searchTextChanged = ImGui::InputTextWithHint("##search", "搜索商品名称、描述或类别...", searchBuffer, IM_ARRAYSIZE(searchBuffer));

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);

    // 搜索按钮
    ImGui::SameLine();
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.0f);
    if (ImGui::Button("搜索", ImVec2(70, 40)) || searchTextChanged)
    {
        if (searchTextChanged || strcmp(searchBuffer, lastSearchBuffer) != 0)
        {
            strcpy_s(lastSearchBuffer, sizeof(lastSearchBuffer), searchBuffer);
            if (strlen(searchBuffer) == 0)
            {
                searchResults.clear();
            }
            else
            {
                performSearch();
            }
        }
    }

    // 清除按钮
    ImGui::SameLine();
    if (ImGui::Button("清除", ImVec2(70, 40)))
    {
        searchResults.clear();
        memset(searchBuffer, 0, sizeof(searchBuffer));
        memset(lastSearchBuffer, 0, sizeof(lastSearchBuffer));
        refreshProducts();
    }
    ImGui::PopStyleVar();
}

// 渲染现代化产品卡片
void ClientUI::renderProductCard(const Protocol::ProductData &product)
{
    ImGui::PushID(product.id.c_str());

    // 卡片容器
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.16f, 0.17f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));

    float cardWidth = 280.0f;
    float cardHeight = 200.0f;

    if (ImGui::BeginChild("ProductCard", ImVec2(cardWidth, cardHeight), true))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));
        // 产品名称 - 使用较大字体
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // 使用默认字体
        ImGui::PushStyleColor(ImGuiCol_Text, this->primaryColor);
        ImGui::TextWrapped("%s", product.name.c_str());
        ImGui::PopStyleColor();
        ImGui::PopFont();

        ImGui::Spacing();

        // 价格信息
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
        if (product.discountRate > 0.0f)
        {
            float discountedPrice = product.originalPrice * (1.0f - product.discountRate);
            ImGui::Text("💰 %.2f元", discountedPrice);
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::Text("(原价: %.2f元)", product.originalPrice);
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::Text("💰 %.2f元", product.originalPrice);
        }
        ImGui::PopStyleColor();

        // 库存信息
        ImGui::PushStyleColor(ImGuiCol_Text, product.quantity > 10 ? ImVec4(0.4f, 0.8f, 0.4f, 1.0f) : ImVec4(0.8f, 0.6f, 0.4f, 1.0f));
        ImGui::Text("📦 库存: %d", product.quantity);
        ImGui::PopStyleColor();

        // 商家信息
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::Text("🏪 %s", product.sellerUsername.c_str());
        ImGui::PopStyleColor();

        // 产品描述
        if (!product.description.empty())
        {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
            ImGui::TextWrapped("%s", product.description.substr(0, 50).c_str());
            if (product.description.length() > 50)
            {
                ImGui::SameLine();
                ImGui::Text("...");
            }
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();

        // 操作按钮
        if (isLoggedIn && userType == Protocol::UserType::CUSTOMER && product.quantity > 0)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

            // 加入购物车按钮
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 1.0f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 1.0f, 1.0f));
            if (ImGui::Button("🛒 加入购物车", ImVec2(120, 30)))
            {
                addProductToCart(product);
            }
            ImGui::PopStyleColor(2);

            ImGui::SameLine();

            // 立即购买按钮
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.4f, 0.4f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));
            if (ImGui::Button("💳 立即购买", ImVec2(120, 30)))
            {
                productToPurchase = product;
                quantityToPurchase = 1;
                showDirectPurchaseConfirmDialog = true;
            }
            ImGui::PopStyleColor(2);

            ImGui::PopStyleVar();
        }
        else if (product.quantity == 0)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.4f, 0.4f, 1.0f));
            ImGui::Text("❌ 暂时缺货");
            ImGui::PopStyleColor();
        }

        ImGui::PopStyleVar(); // ItemSpacing
    }
    ImGui::EndChild();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::PopID();
}

// 渲染现代化状态栏
void ClientUI::renderStatusBar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 10.0f));

    if (ImGui::BeginChild("StatusBar", ImVec2(0, 40), true))
    {
        if (isLoggedIn)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
            ImGui::Text("当前用户：%s", currentUser.username.c_str());
            ImGui::PopStyleColor();

            if (userType == Protocol::UserType::CUSTOMER)
            {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                ImGui::Text("余额: %.2f元", currentUser.balance);
                ImGui::PopStyleColor();
            }
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.6f, 0.4f, 1.0f));
            ImGui::Text("未登录");
            ImGui::PopStyleColor();
        }

        // 显示状态消息
        if (!statusMessage.empty())
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
            ImGui::Text("%s", statusMessage.c_str());
            ImGui::PopStyleColor();
        }

        if (showErrorMessage && !errorMessage.empty())
        {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.4f, 0.4f, 1.0f));
            ImGui::Text("%s", errorMessage.c_str());
            ImGui::PopStyleColor();
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleVar();
}

// 渲染购买确认对话框
void ClientUI::renderPurchaseConfirmDialog()
{
    if (!showPurchaseConfirmDialog)
        return;

    ImGui::OpenPopup("确认购买");
    if (ImGui::BeginPopupModal("确认购买", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // 显示详细的商品信息
        ImGui::Text("订单详情");
        ImGui::Separator();

        ImGui::Text("商品名称：%s", productToPurchase.name.c_str());
        ImGui::Text("商品描述：%s", productToPurchase.description.c_str());
        ImGui::Text("商品类型：%s", productToPurchase.type.c_str());
        ImGui::Text("商    家：%s", productToPurchase.sellerUsername.c_str());
        ImGui::Text("单    价：CNY %.2f", productToPurchase.price);
        ImGui::Text("数    量：%d", quantityToPurchase);

        ImGui::Separator();

        double totalAmount = productToPurchase.price * quantityToPurchase;
        ImGui::Text("总金额：CNY %.2f", totalAmount);
        ImGui::Text("当前余额：CNY %.2f", currentUser.balance);
        ImGui::Text("支付后余额：CNY %.2f", currentUser.balance - totalAmount);

        ImGui::Separator();

        if (ImGui::Button("确认购买", ImVec2(90, 0)))
        {
            purchaseProduct(productToPurchase.name, quantityToPurchase);
            showPurchaseConfirmDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(80, 0)))
        {
            showPurchaseConfirmDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// 渲染直接购买确认对话框
void ClientUI::renderDirectPurchaseConfirmDialog()
{
    if (!showDirectPurchaseConfirmDialog)
        return;

    ImGui::OpenPopup("直接购买确认");
    if (ImGui::BeginPopupModal("直接购买确认", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // 显示详细的商品信息
        ImGui::Text("直接购买订单详情");
        ImGui::Separator();

        ImGui::Text("商品名称：%s", productToPurchase.name.c_str());
        ImGui::Text("商品描述：%s", productToPurchase.description.c_str());
        ImGui::Text("商品类型：%s", productToPurchase.type.c_str());
        ImGui::Text("商    家：%s", productToPurchase.sellerUsername.c_str());
        ImGui::Text("库存数量：%d", productToPurchase.quantity);
        ImGui::Text("单    价：CNY %.2f", productToPurchase.price);
        ImGui::Text("购买数量：%d", quantityToPurchase);

        ImGui::Separator();

        double totalAmount = productToPurchase.price * quantityToPurchase;
        ImGui::Text("总金额：CNY %.2f", totalAmount);
        ImGui::Text("当前余额：CNY %.2f", currentUser.balance);

        if (currentUser.balance >= totalAmount)
        {
            ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "支付后余额：CNY %.2f", currentUser.balance - totalAmount);
        }
        else
        {
            ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.0f, 1.0f), "余额不足！还需：CNY %.2f", totalAmount - currentUser.balance);
        }

        ImGui::Separator();

        bool canPurchase = (currentUser.balance >= totalAmount) && (productToPurchase.quantity >= quantityToPurchase);

        if (!canPurchase)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        }

        if (ImGui::Button("确认购买", ImVec2(90, 0)) && canPurchase)
        {
            purchaseProduct(productToPurchase.name, quantityToPurchase);
            showDirectPurchaseConfirmDialog = false;
            ImGui::CloseCurrentPopup();
        }

        if (!canPurchase)
        {
            ImGui::PopStyleColor(3);
        }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(80, 0)))
        {
            // 取消购买时需要解锁库存
            if (inventoryLocked)
            {
                if (networkClient->unlockInventory(productToPurchase.id, quantityToPurchase))
                {
                    inventoryLocked = false;
                    std::cout << "取消购买，库存已解锁" << std::endl;
                }
                else
                {
                    setError("警告：取消购买时库存解锁失败！请联系客服处理");
                }
            }
            showDirectPurchaseConfirmDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// 渲染购物车结账确认对话框
void ClientUI::renderCartCheckoutConfirmDialog()
{
    if (!showCartCheckoutConfirmDialog)
        return;

    ImGui::OpenPopup("购物车结账确认");
    if (ImGui::BeginPopupModal("购物车结账确认", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize))
    {
        ImGui::SetWindowSize(ImVec2(500, 0), ImGuiCond_Always);
        ImGui::Text("购物车结账订单详情");
        ImGui::Separator();

        // 计算总金额和检查库存状态
        double totalAmount = 0.0;
        bool hasStockIssues = false;

        // 显示商品列表
        if (ImGui::BeginTable("CheckoutTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 150)))
        {
            ImGui::TableSetupColumn("商品名称", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("数量", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("单价", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("小计", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("商家", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableHeadersRow();

            for (const auto &item : cartItems)
            {
                ImGui::TableNextRow();

                // 检查库存状态
                bool stockAvailable = true;
                Protocol::ProductData *product = nullptr;
                for (auto &p : allProducts)
                {
                    if (p.id == item.productId)
                    {
                        product = &p;
                        stockAvailable = (p.quantity >= item.quantity);
                        break;
                    }
                }

                if (!stockAvailable)
                {
                    hasStockIssues = true;
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(1.0f, 0.8f, 0.8f, 0.3f)));
                }

                ImGui::TableSetColumnIndex(0);
                if (!stockAvailable)
                {
                    ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.0f, 1.0f), "%s", item.productName.c_str());
                }
                else
                {
                    ImGui::Text("%s", item.productName.c_str());
                }

                ImGui::TableSetColumnIndex(1);
                if (!stockAvailable && product)
                {
                    ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.0f, 1.0f), "%d (库存:%d)", item.quantity, product->quantity);
                }
                else
                {
                    ImGui::Text("%d", item.quantity);
                }

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("CNY%.2f", item.priceAtAddition);

                ImGui::TableSetColumnIndex(3);
                double subtotal = item.priceAtAddition * item.quantity;
                ImGui::Text("CNY%.2f", subtotal);
                totalAmount += subtotal;

                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%s", item.sellerUsername.c_str());
            }

            ImGui::EndTable();
        }

        ImGui::Separator();

        // 显示总计和余额信息
        ImGui::Text("订单总计：CNY %.2f", totalAmount);
        ImGui::Text("当前余额：CNY %.2f", currentUser.balance);

        if (currentUser.balance >= totalAmount)
        {
            ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "支付后余额：CNY %.2f", currentUser.balance - totalAmount);
        }
        else
        {
            ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.0f, 1.0f), "余额不足！还需：CNY %.2f", totalAmount - currentUser.balance);
        }

        // 显示库存警告
        if (hasStockIssues)
        {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f, 0.0f, 0.0f, 1.0f), "⚠ 部分商品库存不足，请调整购买数量");
        }

        ImGui::Separator();

        bool canCheckout = (currentUser.balance >= totalAmount) && !hasStockIssues;

        if (!canCheckout)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        }

        if (ImGui::Button("确认结账", ImVec2(100, 0)) && canCheckout)
        {
            checkoutWithInventoryLock();
            showCartCheckoutConfirmDialog = false;
            ImGui::CloseCurrentPopup();
        }

        if (!canCheckout)
        {
            ImGui::PopStyleColor(3);
        }

        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(80, 0)))
        {
            showCartCheckoutConfirmDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// 渲染添加到购物车成功弹窗
void ClientUI::renderAddToCartSuccessPopup()
{
    if (!showAddToCartSuccessPopup)
        return;

    ImGui::OpenPopup("添加成功");
    if (ImGui::BeginPopupModal("添加成功", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("%s", addToCartSuccessMessage.c_str());

        ImGui::Separator();
        if (ImGui::Button("确定", ImVec2(80, 0)))
        {
            showAddToCartSuccessPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// 渲染直接购买成功弹窗
void ClientUI::renderDirectPurchaseSuccessPopup()
{
    if (!showDirectPurchaseSuccessPopup)
        return;

    ImGui::OpenPopup("购买成功");
    if (ImGui::BeginPopupModal("购买成功", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("%s", directPurchaseSuccessMessage.c_str());

        ImGui::Separator();

        if (ImGui::Button("确定", ImVec2(80, 0)))
        {
            showDirectPurchaseSuccessPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// 渲染密码修改对话框
void ClientUI::renderPasswordChangeDialog()
{
    if (!showPasswordChangeDialog)
        return;

    static char oldPassword[128] = {0};
    static char newPassword[128] = {0};
    static char confirmPassword[128] = {0};

    ImGui::OpenPopup("修改密码");
    if (ImGui::BeginPopupModal("修改密码", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("用户：%s", currentUser.username.c_str());
        ImGui::Separator();

        ImGui::InputText("原密码", oldPassword, sizeof(oldPassword), ImGuiInputTextFlags_Password);
        ImGui::InputText("新密码", newPassword, sizeof(newPassword), ImGuiInputTextFlags_Password);
        ImGui::InputText("确认新密码", confirmPassword, sizeof(confirmPassword), ImGuiInputTextFlags_Password);

        ImGui::Separator();

        if (ImGui::Button("确认修改", ImVec2(100, 0)))
        {
            if (strcmp(newPassword, confirmPassword) == 0)
            {
                // 这里应该调用网络API修改密码
                showPasswordChangeSuccessDialog = true;
                showPasswordChangeDialog = false;
                memset(oldPassword, 0, sizeof(oldPassword));
                memset(newPassword, 0, sizeof(newPassword));
                memset(confirmPassword, 0, sizeof(confirmPassword));
                ImGui::CloseCurrentPopup();
            }
            else
            {
                showPasswordChangeFailDialog = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(80, 0)))
        {
            showPasswordChangeDialog = false;
            memset(oldPassword, 0, sizeof(oldPassword));
            memset(newPassword, 0, sizeof(newPassword));
            memset(confirmPassword, 0, sizeof(confirmPassword));
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// 渲染密码修改成功对话框
void ClientUI::renderPasswordChangeSuccessDialog()
{
    if (!showPasswordChangeSuccessDialog)
        return;

    ImGui::OpenPopup("修改成功");
    if (ImGui::BeginPopupModal("修改成功", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("密码修改成功！");

        ImGui::Separator();

        if (ImGui::Button("确定", ImVec2(80, 0)))
        {
            showPasswordChangeSuccessDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// 渲染密码修改失败对话框
void ClientUI::renderPasswordChangeFailDialog()
{
    if (!showPasswordChangeFailDialog)
        return;

    ImGui::OpenPopup("修改失败");
    if (ImGui::BeginPopupModal("修改失败", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("密码修改失败！请检查输入。");

        ImGui::Separator();

        if (ImGui::Button("确定", ImVec2(80, 0)))
        {
            showPasswordChangeFailDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// 渲染充值对话框
void ClientUI::renderRechargeDialog()
{
    if (!showRechargeDialog)
        return;

    ImGui::OpenPopup("账户充值");
    if (ImGui::BeginPopupModal("账户充值", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("当前余额：%.2f 元", currentUser.balance);
        ImGui::Separator();

        ImGui::InputFloat("充值金额", &rechargeAmount, 1.0f, 10.0f, "%.2f");

        if (rechargeAmount < 0)
            rechargeAmount = 0;

        ImGui::Separator();

        if (ImGui::Button("确认充值", ImVec2(100, 0)))
        {
            if (rechargeAmount > 0)
            {
                // 这里应该调用网络API进行充值
                currentUser.balance += rechargeAmount;
                showRechargeSuccessDialog = true;
                showRechargeDialog = false;
                rechargeAmount = 0.0f;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("取消", ImVec2(80, 0)))
        {
            showRechargeDialog = false;
            rechargeAmount = 0.0f;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// 渲染充值成功对话框
void ClientUI::renderRechargeSuccessDialog()
{
    if (!showRechargeSuccessDialog)
        return;

    ImGui::OpenPopup("充值成功");
    if (ImGui::BeginPopupModal("充值成功", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("充值成功！");
        ImGui::Text("当前余额：%.2f 元", currentUser.balance);

        ImGui::Separator();

        if (ImGui::Button("确定", ImVec2(80, 0)))
        {
            showRechargeSuccessDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// 渲染充值失败对话框
void ClientUI::renderRechargeFailDialog()
{
    if (!showRechargeFailDialog)
        return;

    ImGui::OpenPopup("充值失败");
    if (ImGui::BeginPopupModal("充值失败", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("充值失败！%s", rechargeMessage.c_str());

        ImGui::Separator();
        if (ImGui::Button("确定", ImVec2(80, 0)))
        {
            showRechargeFailDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// 窗口响应式功能实现

// 更新窗口尺寸
void ClientUI::updateWindowSize()
{
    int newWidth, newHeight;
    glfwGetWindowSize(window, &newWidth, &newHeight);

    if (newWidth != width || newHeight != height)
    {
        width = newWidth;
        height = newHeight;
        updateUIScale();
    }
}

// 更新UI缩放比例
void ClientUI::updateUIScale()
{
    // 计算基于宽度和高度的缩放比例
    float scaleX = static_cast<float>(width) / static_cast<float>(originalWidth);
    float scaleY = static_cast<float>(height) / static_cast<float>(originalHeight);

    // 使用较小的缩放比例来确保UI元素不会超出窗口
    uiScale = std::min(scaleX, scaleY);

    // 限制缩放比例范围，避免过小或过大
    uiScale = std::max(0.5f, std::min(uiScale, 3.0f));

    // 更新ImGui样式缩放
    ImGuiStyle &style = ImGui::GetStyle();
    ImGuiIO &io = ImGui::GetIO();

    // 缩放字体
    io.FontGlobalScale = uiScale;

    // 缩放ImGui样式元素
    style.WindowPadding = ImVec2(15.0f * uiScale, 15.0f * uiScale);
    style.FramePadding = ImVec2(12.0f * uiScale, 8.0f * uiScale);
    style.ItemSpacing = ImVec2(12.0f * uiScale, 8.0f * uiScale);
    style.ItemInnerSpacing = ImVec2(8.0f * uiScale, 6.0f * uiScale);
    style.IndentSpacing = 25.0f * uiScale;
    style.ScrollbarSize = 15.0f * uiScale;
    style.GrabMinSize = 10.0f * uiScale;

    // 缩放圆角
    style.WindowRounding = 10.0f * uiScale;
    style.ChildRounding = 8.0f * uiScale;
    style.FrameRounding = 6.0f * uiScale;
    style.PopupRounding = 8.0f * uiScale;
    style.ScrollbarRounding = 6.0f * uiScale;
    style.GrabRounding = 6.0f * uiScale;
    style.TabRounding = 6.0f * uiScale;
}

// 缩放尺寸向量
ImVec2 ClientUI::scaleSize(const ImVec2 &size)
{
    return ImVec2(size.x * uiScale, size.y * uiScale);
}

// 缩放浮点值
float ClientUI::scaleFloat(float value)
{
    return value * uiScale;
}

// GLFW窗口大小回调函数
void ClientUI::framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    // 获取用户指针（ClientUI实例）
    ClientUI *clientUI = static_cast<ClientUI *>(glfwGetWindowUserPointer(window));
    if (clientUI)
    {
        clientUI->updateWindowSize();

        // 更新OpenGL视口
        glViewport(0, 0, width, height);
    }
}