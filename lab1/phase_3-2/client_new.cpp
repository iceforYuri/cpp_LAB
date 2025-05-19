#include "client.h"

// Client类的构造函数
Client::Client() 
    : clientSocket(INVALID_SOCKET), 
      currentUsername(""), 
      currentUserType(""), 
      isLoggedIn(false) {
}

// Client类的析构函数
Client::~Client() {
    // 确保断开连接
    if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
    WSACleanup();
}

// 连接到服务器
bool Client::connectToServer() {
    // 创建客户端socket
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "创建socket失败: " << WSAGetLastError() << std::endl;
        return false;
    }

    // 设置服务器地址
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    // 默认连接到本地服务器，可以根据需要修改
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
    serverAddr.sin_port = htons(8888); // 使用8888端口

    // 连接到服务器
    if (connect(clientSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "连接失败: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        return false;
    }

    std::cout << "已连接到服务器。" << std::endl;
    return true;
}

// 辅助函数：清除输入缓冲区
void Client::clearInputBuffer() const {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// 发送请求到服务器
bool Client::sendRequest(const std::string &request, std::string &response) const {
    // 发送请求
    if (send(clientSocket, request.c_str(), request.size(), 0) == SOCKET_ERROR) {
        std::cerr << "发送失败: " << WSAGetLastError() << std::endl;
        return false;
    }

    // 接收响应
    char buffer[4096] = {0};
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        std::cerr << "接收失败或连接关闭: " << WSAGetLastError() << std::endl;
        return false;
    }

    response = std::string(buffer, bytesReceived);
    return true;
}

// 显示主菜单
int Client::showMainMenu() const {
    int choice;

    std::cout << "\n===== 欢迎使用网络电子商城系统 =====\n";
    std::cout << "1. 登录\n";
    std::cout << "2. 注册\n";
    std::cout << "0. 退出\n";
    std::cout << "请选择: ";

    std::cin >> choice;
    clearInputBuffer();

    if (choice < 0 || choice > 2) {
        std::cout << "无效选择，请重试。\n";
        return 1; // 默认返回继续
    }

    return choice;
}

// 显示消费者菜单
int Client::showCustomerMenu() const {
    int choice;

    std::cout << "\n\n\n--- 消费者菜单 (" << currentUsername << ") ---\n";
    std::cout << "1. 查看个人信息\n";
    std::cout << "2. 修改密码\n";
    std::cout << "3. 查询余额\n";
    std::cout << "4. 充值\n";
    std::cout << "5. 进入商城\n";
    std::cout << "6. 退出登录\n";
    std::cout << "0. 退出程序\n";
    std::cout << "请选择一个选项: ";

    std::cin >> choice;
    clearInputBuffer();

    if (choice == 6)
        return -1; // 退出登录
    if (choice == 0)
        return 0; // 退出程序
    if (choice < 0 || choice > 6) {
        std::cout << "无效选择，请重试。\n";
        return 1; // 默认返回继续
    }

    return choice;
}

// 显示商家菜单
int Client::showSellerMenu() const {
    int choice;

    std::cout << "\n===== 商家菜单 (" << currentUsername << ") =====\n";
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
        return -1; // 退出登录
    if (choice == 0)
        return 0; // 退出程序
    if (choice < 0 || choice > 5) {
        std::cout << "无效选择，请重试。\n";
        return 1; // 默认返回继续
    }

    return choice;
}

// 处理登录
void Client::handleLogin() {
    std::string username, password;

    std::cout << "\n===== 用户登录 =====\n";
    std::cout << "用户名: ";
    std::getline(std::cin, username);
    std::cout << "密码: ";
    std::getline(std::cin, password);

    std::string request = "LOGIN|" + username + "|" + password;
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, message, userType;

        std::getline(iss, status, '|');

        if (status == "LOGIN_SUCCESS") {
            std::getline(iss, message, '|');
            std::getline(iss, userType, '|');

            isLoggedIn = true;
            currentUsername = message;
            currentUserType = userType;

            std::cout << "登录成功！欢迎 " << currentUsername << "(" << currentUserType << ")" << std::endl;
        } else {
            std::getline(iss, message, '|');
            std::cout << "登录失败: " << message << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }
}

// 处理注册
void Client::handleRegister() {
    std::string username, password, confirmPassword, typeChoice;
    int choice;

    std::cout << "\n===== 用户注册 =====\n";
    std::cout << "用户名: ";
    std::getline(std::cin, username);
    std::cout << "密码: ";
    std::getline(std::cin, password);
    std::cout << "确认密码: ";
    std::getline(std::cin, confirmPassword);

    if (password != confirmPassword) {
        std::cout << "两次输入的密码不一致，请重试。" << std::endl;
        return;
    }

    std::cout << "账户类型 (1: 消费者, 2: 商家): ";
    std::cin >> choice;
    clearInputBuffer();

    if (choice == 1) {
        typeChoice = "customer";
    } else if (choice == 2) {
        typeChoice = "seller";
    } else {
        std::cout << "无效选择，请重试。" << std::endl;
        return;
    }

    std::string request = "REGISTER|" + username + "|" + password + "|" + typeChoice;
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, message;

        std::getline(iss, status, '|');
        std::getline(iss, message, '|');

        if (status == "REGISTER_SUCCESS") {
            std::cout << "注册成功！用户名: " << message << std::endl;
        } else {
            std::cout << "注册失败: " << message << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }
}

// 处理登出
void Client::handleLogout() {
    if (!isLoggedIn) {
        return;
    }

    std::string request = "LOGOUT|" + currentUsername;
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, message;

        std::getline(iss, status, '|');
        std::getline(iss, message, '|');

        if (status == "LOGOUT_SUCCESS") {
            std::cout << "已登出: " << message << std::endl;
        } else {
            std::cout << "登出失败: " << message << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败，但本地已登出。" << std::endl;
    }

    // 无论服务器响应如何，都重置本地登录状态
    isLoggedIn = false;
    currentUsername = "";
    currentUserType = "";
}

// Client::run 方法实现 - 客户端主循环
int Client::run() {
    int menuResult = 1; // 1=继续, 0=退出程序, -1=退出登录

    while (menuResult != 0) {
        if (menuResult == -1) { // 处理退出登录
            handleLogout();
            menuResult = 1;
        }

        if (!isLoggedIn) {
            menuResult = showMainMenu();

            if (menuResult == 1) { // 登录
                handleLogin();
            } else if (menuResult == 2) { // 注册
                handleRegister();
            }
        } else { // 已登录
            if (currentUserType == "customer") {
                menuResult = showCustomerMenu();

                if (menuResult == 1) { // 查看个人信息
                    handleViewUserInfo();
                } else if (menuResult == 2) { // 修改密码
                    handleChangePassword();
                } else if (menuResult == 3) { // 查询余额
                    handleCheckBalance();
                } else if (menuResult == 4) { // 充值
                    handleDeposit();
                } else if (menuResult == 5) { // 进入商城
                    handleEnterStore();
                }
            } else if (currentUserType == "seller") {
                menuResult = showSellerMenu();

                if (menuResult == 1) { // 查看商户信息
                    handleSellerInfo();
                } else if (menuResult == 2) { // 修改密码
                    handleChangePassword();
                } else if (menuResult == 3) { // 进入商城(管理)
                    handleManageProducts();
                } else if (menuResult == 4) { // 查看收入
                    handleCheckIncome();
                }
            }
        }
    }
    
    // 确保登出
    if (isLoggedIn) {
        handleLogout();
    }
    
    return 0;
}

// 实现其余函数...
// 为了简洁起见，这里只实现了一些核心函数
// 其他函数可以类似地实现

void Client::handleViewUserInfo() const {
    if (!isLoggedIn) {
        std::cout << "请先登录。" << std::endl;
        return;
    }

    std::string request = "GET_USER_INFO|" + currentUsername;
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status;

        std::getline(iss, status, '|');

        if (status == "USER_INFO") {
            std::string username, userType, balanceStr;
            std::getline(iss, username, '|');
            std::getline(iss, userType, '|');
            std::getline(iss, balanceStr, '|');

            std::cout << "\n===== 用户信息 =====\n";
            std::cout << "用户名: " << username << "\n";
            std::cout << "账户类型: " << userType << "\n";
            std::cout << "账户余额: " << balanceStr << " 元\n";
        } else {
            std::string message;
            std::getline(iss, message, '|');
            std::cout << "获取用户信息失败: " << message << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void Client::handleChangePassword() const {
    if (!isLoggedIn) {
        std::cout << "请先登录。" << std::endl;
        return;
    }

    std::string oldPassword, newPassword;

    std::cout << "\n===== 修改密码 =====\n";
    std::cout << "请输入原密码: ";
    std::getline(std::cin, oldPassword);
    std::cout << "请输入新密码: ";
    std::getline(std::cin, newPassword);

    std::string request = "CHANGE_PASSWORD|" + currentUsername + "|" + oldPassword + "|" + newPassword;
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, message;

        std::getline(iss, status, '|');
        std::getline(iss, message, '|');

        if (status == "PASSWORD_CHANGED") {
            std::cout << "密码修改成功。" << std::endl;
        } else {
            std::cout << "密码修改失败: " << message << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void Client::handleCheckBalance() const {
    if (!isLoggedIn) {
        std::cout << "请先登录。" << std::endl;
        return;
    }

    std::string request = "CHECK_BALANCE|" + currentUsername;
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, balanceStr;

        std::getline(iss, status, '|');
        std::getline(iss, balanceStr, '|');

        if (status == "BALANCE") {
            std::cout << "\n当前余额: " << balanceStr << " 元" << std::endl;
        } else {
            std::cout << "查询余额失败: " << balanceStr << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

void Client::handleDeposit() const {
    if (!isLoggedIn) {
        std::cout << "请先登录。" << std::endl;
        return;
    }

    double amount;
    std::cout << "\n===== 充值 =====\n";
    std::cout << "输入充值金额: ";
    
    if (!(std::cin >> amount) || amount <= 0) {
        std::cout << "无效金额。" << std::endl;
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    std::string request = "DEPOSIT|" + currentUsername + "|" + std::to_string(amount);
    std::string response;

    if (sendRequest(request, response)) {
        std::istringstream iss(response);
        std::string status, message, newBalanceStr;

        std::getline(iss, status, '|');
        std::getline(iss, message, '|');

        if (status == "BALANCE_UPDATED") {
            std::getline(iss, newBalanceStr, '|');
            std::cout << message << " 当前余额: " << newBalanceStr << " 元" << std::endl;
        } else {
            std::cout << "充值失败: " << message << std::endl;
        }
    } else {
        std::cout << "与服务器通信失败。" << std::endl;
    }

    std::cout << "按回车键继续...";
    std::cin.get();
}

// 客户端主程序入口
int main() {
    SetConsoleOutputCP(65001); // 设置控制台输出编码为 UTF-8
    SetConsoleCP(65001);       // 设置控制台输入编码为 UTF-8

    // 初始化 Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup 失败: " << result << std::endl;
        return 1;
    }

    // 创建并运行客户端
    Client client;
    
    // 连接到服务器
    if (!client.connectToServer()) {
        std::cerr << "无法连接到服务器，请检查服务器是否运行" << std::endl;
        WSACleanup();
        return 1;
    }
    
    // 运行客户端主循环
    return client.run();
}
