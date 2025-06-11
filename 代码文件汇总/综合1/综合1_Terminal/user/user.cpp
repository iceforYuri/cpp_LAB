#include "user.h"
#include <iostream>
#include <limits>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

// User 构造函数实现
User::User(string username, string password, double balance, bool customer, bool seller, bool admin)
    : username(username), password(password), balance(balance) {}

User::User()
{
    username = "defaultUser";
    password = "12345678";
    balance = 0.0;
    userType = "customer"; // 默认用户类型为消费者
}

// Customer 构造函数实现
Customer::Customer(string username, string password, double balance)
    : User(username, password, balance, true, false, false) {}

Customer::Customer() : User()
{
    userType = "customer"; // 默认用户类型为消费者
}

// Seller 构造函数实现
Seller::Seller(string username, string password, double balance)
    : User(username, password, balance, false, true, false) {}

Seller::Seller() : User()
{
    userType = "seller"; // 默认用户类型为商家
}

// Admin 构造函数实现
Admin::Admin(string username, string password, double balance)
    : User(username, password, balance, false, false, true) {}

Admin::Admin() : User()
{
    userType = "admin"; // 默认用户类型为管理员
}

// --- 静态方法：用户注册实现 ---
User *User::registerUser(vector<User *> &users)
{
    string username, password;
    int userTypeChoice;

    cout << "\n--- 用户注册 ---" << endl;

    // 获取并验证用户名
    while (true)
    {
        cout << "请输入用户名: ";
        getline(cin >> ws, username); // ws 忽略前导空格

        if (username.empty())
        {
            cout << "用户名不能为空，请重新输入！" << endl;
            continue;
        }
        // 调用 User 类的静态方法检查用户名是否存在
        if (User::isUsernameExists(users, username))
        {
            cout << "该用户名已存在，请选择其他用户名！" << endl;
            continue;
        }
        break;
    }

    // 获取密码
    cout << "请输入密码: ";
    getline(cin >> ws, password);

    // 选择用户类型
    cout << "请选择用户类型 (1: 消费者, 2: 商家): ";
    while (!(cin >> userTypeChoice) || (userTypeChoice != 1 && userTypeChoice != 2))
    {
        cout << "无效选择，请输入 1 或 2: ";
        User::clearInputBuffer(); // 调用 User 类的静态 clearInputBuffer
    }
    User::clearInputBuffer(); // 清除 cin 留下的换行符

    User *newUser = nullptr;
    // 根据选择创建 Customer 或 Seller 对象 (User 的派生类)
    if (userTypeChoice == 1)
    {
        newUser = new Customer(username, password);
    }
    else // userTypeChoice == 2
    {
        newUser = new Seller(username, password);
    }

    users.push_back(newUser); // 将新用户指针添加到向量

    // 注意：这里不再自动保存文件，让调用者决定何时保存
    // User::saveUsersToFile(users, USER_FILE); // 移除或注释掉

    cout << "注册成功！" << endl;
    return newUser; // 返回新注册的用户指针
}

// --- 静态方法：用户登录实现 ---
User *User::userLogin(const vector<User *> &users)
{
    string username, password;

    cout << "\n--- 用户登录 ---" << endl;
    cout << "请输入用户名: ";
    getline(cin >> ws, username);

    cout << "请输入密码: ";
    getline(cin >> ws, password);

    User *user = findUser(users, username);
    if (user == nullptr)
    {
        cout << "用户不存在！" << endl;
        return nullptr;
    }
    if (user->getPassword() != password)
    {
        cout << "密码错误！" << endl;
        return nullptr;
    }
    cout << "登录成功！" << endl;
    cout << "欢迎回来，" << user->getUsername() << "！" << endl;
    return user; // 返回登录成功的用户指针
}

// 显示单个用户信息
void User::displayInfo(int index) const
{
    if (index > 0)
    {
        cout << "用户 " << index << ":" << endl;
    }
    cout << "  用户名: " << username << endl;
    cout << "  密码: " << password << endl;
    cout << "  用户类型: " << getUserType() << endl;
    cout << "  账户余额: " << balance << endl;
    cout << "---------------------" << endl;

    // 需改为按下enter也能返回
    cout << "按任意键返回" << endl;
    cin.get();                                           // 等待用户输入
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // 忽略剩余输入
}

// 静态方法：显示所有用户信息
void User::displayAllUsers(const vector<User *> &users)
{
    cout << "\n--- 所有用户信息 ---" << endl;
    if (users.empty())
    {
        cout << "当前没有已注册的用户。" << endl;
    }
    else
    {
        for (size_t i = 0; i < users.size(); ++i)
        {
            users[i]->displayInfo(i + 1);
        }
    }
}

// 修改密码
bool User::changePassword(const string &oldPassword, const string &newPassword)
{
    if (password != oldPassword)
    {
        cout << "原密码错误！" << endl;
        return false;
    }

    password = newPassword;
    cout << "密码修改成功！" << endl;
    return true;
}

// 充值余额
bool User::deposit(double amount)
{
    if (amount <= 0)
    {
        cout << "充值金额必须大于0！" << endl;
        return false;
    }

    balance += amount;
    // cout << "充值成功！当前余额: " << balance << endl;
    return true;
}

// 消费余额
bool User::withdraw(double amount)
{
    if (amount <= 0)
    {
        cout << "消费金额必须大于0！" << endl;
        return false;
    }

    if (balance < amount)
    {
        cout << "余额不足！当前余额: " << balance << endl;
        return false;
    }

    balance -= amount;
    cout << "支付成功！当前余额: " << balance << endl;
    return true;
}

// 查询余额
double User::checkBalance() const
{
    return balance;
}

// 将用户信息保存到文件
bool User::saveUsersToFile(const vector<User *> &users, const string &filename)
{
    ofstream file(filename);
    if (!file.is_open())
    {
        cerr << "无法打开文件: " << filename << endl;
        return false;
    }

    for (const auto &user : users)
    {
        file << user->getUserType() << ","
             << user->getUsername() << ","
             << user->getPassword() << ","
             << user->checkBalance() << endl;
    }

    file.close();
    return true;
}

// 从文件加载用户信息
vector<User *> User::loadUsersFromFile(const string &filename)
{
    vector<User *> users;
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "无法打开文件: " << filename << " (可能是首次运行)" << endl;
        return users;
    }

    string line;
    while (getline(file, line))
    {
        stringstream ss(line);
        string userType, username, password;
        double balance;

        getline(ss, userType, ',');
        getline(ss, username, ',');
        getline(ss, password, ',');
        ss >> balance;

        User *user = nullptr;
        if (userType == "customer")
        {
            user = new Customer(username, password, balance);
        }
        else if (userType == "seller")
        {
            user = new Seller(username, password, balance);
        }
        else if (userType == "admin")
        {
            user = new Admin(username, password, balance);
        }

        if (user != nullptr)
        {
            users.push_back(user);
        }
    }

    file.close();
    return users;
}

// 查找指定用户名的用户
User *User::findUser(const vector<User *> &users, const string &username)
{
    for (auto user : users)
    {
        if (user->getUsername() == username)
        {
            return user;
        }
    }
    return nullptr;
}

// 检查用户名是否已存在
bool User::isUsernameExists(const vector<User *> &users, const string &username)
{
    return findUser(users, username) != nullptr;
}

// 用户登录验证
User *User::login(const vector<User *> &users, const string &username, const string &password)
{
    User *user = findUser(users, username);
    if (user && user->getPassword() == password)
    {
        return user;
    }
    return nullptr;
}

// Getter 函数实现
string User::getUsername() const { return username; }
string User::getPassword() const { return password; }

// Setter 函数实现
void User::setUsername(const string &newUsername) { username = newUsername; }
void User::setPassword(const string &newPassword) { password = newPassword; }
void User::setBalance(double newBalance) { balance = newBalance; }