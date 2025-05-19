#include "client_ui.h"
#include "client_app.h"

ClientUI::ClientUI(ClientApp *app) : m_app(app)
{
}

int ClientUI::showMainMenu() const
{
    int choice;

    std::cout << "\n===== 电子商城系统 =====\n";
    std::cout << "1. 登录\n";
    std::cout << "2. 注册\n";
    std::cout << "0. 退出程序\n";
    std::cout << "请选择: ";

    std::cin >> choice;
    clearInputBuffer();

    return choice;
}

int ClientUI::showCustomerMenu(const std::string &username) const
{
    int choice;

    std::cout << "\n===== 消费者菜单 (" << username << ") =====\n";
    std::cout << "1. 查看个人信息\n";
    std::cout << "2. 查询余额\n";
    std::cout << "3. 充值\n";
    std::cout << "4. 进入商城\n";
    std::cout << "5. 查看购物车\n";
    std::cout << "6. 退出登录\n";
    std::cout << "0. 退出程序\n";
    std::cout << "请选择: ";

    std::cin >> choice;
    clearInputBuffer();

    if (choice == 6)
    {
        return 6; // 退出登录
    }
    if (choice == 0)
    {
        return 0; // 退出程序
    }
    if (choice < 0 || choice > 6)
    {
        std::cout << "无效选择，请重试。" << std::endl;
    }

    return choice;
}

int ClientUI::showSellerMenu(const std::string &username) const
{
    int choice;

    std::cout << "\n===== 商家菜单 (" << username << ") =====\n";
    std::cout << "1. 查看商户信息\n";
    std::cout << "2. 修改密码\n";
    std::cout << "3. 进入商城 (管理)\n";
    std::cout << "4. 查看收入\n";
    std::cout << "5. 退出登录\n";
    std::cout << "0. 退出程序\n";
    std::cout << "请选择: ";

    std::cin >> choice;
    clearInputBuffer();

    if (choice == 5)
    {
        return 5; // 退出登录
    }
    if (choice == 0)
    {
        return 0; // 退出程序
    }
    if (choice < 0 || choice > 5)
    {
        std::cout << "无效选择，请重试。" << std::endl;
    }

    return choice;
}

void ClientUI::clearInputBuffer() const
{
    std::cin.clear();                                                   // 清除错误标志
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 忽略缓冲区中的所有字符直到换行符
}
