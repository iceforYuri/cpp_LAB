#include "page.h"
#include "../user/user.h"
#include "../store/store.h" // 需要 Store 定义
#include <iostream>
#include <cstdio> // for printf if used
#include <limits>
#include <vector>
#include <string>
#include <iomanip>

using namespace std;

extern const string USER_FILE;
// STORE_FILE is managed by the Store object passed in

// --- 基类 Page 静态方法 ---
void Page::clearInputBuffer()
{
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// --- 基类 Page 菜单实现 ---
// 这些函数现在只处理用户账户菜单，并在选择商城时返回特定值 3

int Page::pagemain(vector<User *> &users, User *&currentUser)
{
    int choice = 0;
    while (true)
    {
        printf("\n\n\n--- 主菜单 ---\n");
        cout << "1. 注册新用户" << endl;
        cout << "2. 用户登录" << endl;
        cout << "3. 进入商城 (游客)" << endl; // 返回 3 表示进入商城
        cout << "4. 退出程序" << endl;
        cout << "请选择一个选项: ";
        if (!(cin >> choice))
        {
            cout << "无效输入。" << endl;
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();
        switch (choice)
        {
        case 1:
            currentUser = User::registerUser(users);
            if (currentUser)
            {
                User::saveUsersToFile(users, USER_FILE);
                // 注册成功后可以选择留在主菜单或直接进入用户菜单
                // return 1; // 如果想直接进入用户菜单
            }
            return 1;
            break; // 留在主菜单
        case 2:
            currentUser = User::userLogin(users);
            if (currentUser)
                return 1; // 登录成功，返回 1 给 main 处理
            break;        // 登录失败，留在主菜单
        case 3:
            return 3; // 返回 3，通知 main 调用 pagestore
        case 4:
            cout << "退出程序..." << endl;
            return 0; // 返回 0 给 main 退出循环
        default:
            cout << "无效选项。" << endl;
        }
    }
}

int Page::pagemainCustomer(User *currentUser, vector<User *> &users)
{
    int choice = 0;
    while (true)
    {
        printf("\n\n\n--- 消费者菜单 (%s) ---\n", currentUser->getUsername().c_str());
        cout << "1. 查看个人信息" << endl;
        cout << "2. 修改密码" << endl;
        cout << "3. 查询余额" << endl;
        cout << "4. 充值" << endl;
        cout << "5. 进入商城" << endl; // 返回 3 表示进入商城
        cout << "6. 退出登录" << endl;
        cout << "请选择一个选项: ";
        if (!(cin >> choice))
        {
            cout << "无效输入。" << endl;
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();
        switch (choice)
        {
        case 1:
            currentUser->displayInfo();
            break;
        case 2:
        {
            string oldP, newP;
            cout << "请输入原密码: ";
            getline(cin >> ws, oldP);
            cout << "请输入新密码: ";
            getline(cin >> ws, newP);
            if (currentUser->changePassword(oldP, newP))
            {
                User::saveUsersToFile(users, USER_FILE);
                cout << "密码修改成功。" << endl;
            }
            else
            {
                cout << "密码修改失败。" << endl;
            }
        }
        break;
        case 3:
        {
            cout << fixed << setprecision(2); // 设置输出格式
            cout << "当前余额: " << currentUser->checkBalance() << endl;
            break;
        }
        case 4:
        {
            double amt;
            cout << "请输入充值金额: ";
            if (!(cin >> amt) || amt <= 0)
            { // 检查无效输入和非正数
                cout << "无效金额。" << endl;
                clearInputBuffer();
                break;
            }
            clearInputBuffer();
            if (currentUser->deposit(amt))
            {
                User::saveUsersToFile(users, USER_FILE);
                cout << "充值成功。当前余额: " << currentUser->checkBalance() << endl;
            }
            else
            {
                // deposit 内部应该已经打印了错误信息
            }
        }
        break;
        case 5:
            return 3; // 返回 3，通知 main 调用 pagestore
        case 6:
            cout << "退出登录..." << endl;
            return 2; // 返回 2 给 main 处理退出登录
        default:
            cout << "无效选项。" << endl;
        }
    }
}

int Page::pagemainSeller(User *currentUser, vector<User *> &users)
{
    int choice = 0;
    while (true)
    {
        printf("\n\n\n--- 商家菜单 (%s) ---\n", currentUser->getUsername().c_str());
        cout << "1. 查看商户信息" << endl;
        cout << "2. 修改密码" << endl;
        cout << "3. 进入商城 (管理)" << endl; // 返回 3 表示进入商城
        cout << "4. 查看收入" << endl;
        cout << "0. 退出登录" << endl;
        cout << "请选择一个选项: ";
        if (!(cin >> choice))
        {
            cout << "无效输入。" << endl;
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();
        switch (choice)
        {
        case 1:
            currentUser->displayInfo();
            break;
        case 2:
        {
            string oldP, newP;
            cout << "请输入原密码: ";
            getline(cin >> ws, oldP);
            cout << "请输入新密码: ";
            getline(cin >> ws, newP);
            if (currentUser->changePassword(oldP, newP))
            {
                User::saveUsersToFile(users, USER_FILE);
                cout << "密码修改成功。" << endl;
            }
            else
            {
                cout << "密码修改失败。" << endl;
            }
        }
        break;
        case 3:
            return 3; // 返回 3，通知 main 调用 pagestore
        case 4:
        {
            double income = currentUser->checkBalance(); // 假设收入就是余额
            cout << fixed << setprecision(2);            // 设置输出格式
            cout << "当前收入: " << income << endl;
            break;
        }
        case 0:
            cout << "退出登录..." << endl;
            return 2; // 返回 2 给 main 处理退出登录
        default:
            cout << "无效选项。" << endl;
        }
    }
}

int Page::pagemainAdmin(User *currentUser, vector<User *> &users)
{
    // 管理员菜单实现
    int choice = 0;
    while (true)
    {
        printf("\n\n\n--- 管理员菜单 (%s) ---\n", currentUser->getUsername().c_str());
        cout << "1. 查看所有用户信息" << endl;
        // 可以添加其他管理员功能
        cout << "2. 退出登录" << endl;
        cout << "请选择一个选项: ";
        if (!(cin >> choice))
        {
            cout << "无效输入。" << endl;
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();
        switch (choice)
        {
        case 1:
            User::displayAllUsers(users); // 假设 User 类有这个静态方法
            break;
        case 2:
            cout << "退出登录..." << endl;
            return 2; // 返回 2 给 main 处理退出登录
        default:
            cout << "无效选项。" << endl;
        }
    }
}

// --- Customer_page 商城实现 ---
int Customer_page::pagestore(User *currentUser, std::vector<User *> &users, Store &store)
{
    int choice = -1;
    Customer *customer = dynamic_cast<Customer *>(currentUser); // 用于调用购物车方法

    string username_display = "游客";
    if (currentUser)
    {
        username_display = currentUser->getUsername();
    }

    do
    {
        cout << "\n--- 商城 (用户: " << username_display << ") ---" << endl;
        cout << "1. 显示所有商品" << endl;
        cout << "2. 按名称搜索商品" << endl;
        cout << "3. 直接购买商品" << endl; // 保持不变
        cout << "4. 加入购物车" << endl;
        cout << "5. 查看我的购物车" << endl;
        cout << "0. 退出商城" << endl;
        cout << "请选择: ";
        if (!(cin >> choice))
        {
            cout << "无效输入。" << endl;
            Page::clearInputBuffer();
            choice = -1;
            continue;
        }
        Page::clearInputBuffer();

        switch (choice)
        {
        case 1:
            store.displayAllProducts();
            break;
        case 2:
        {
            string searchTerm;
            cout << "输入搜索名称: ";
            getline(cin >> ws, searchTerm);
            vector<Product *> results = store.searchProductsByName(searchTerm);
            if (results.empty())
            {
                cout << "未找到匹配商品。" << endl;
            }
            else
            {
                cout << "\n--- 搜索结果 ---" << endl;
                for (const auto *p : results)
                {
                    cout << "-----------------" << endl;
                    p->display();
                }
                cout << "-----------------" << endl;
            }
            break;
        }
        case 3: // 直接购买商品逻辑 (与你提供的版本类似，确保 currentUser 是 Customer)
        {
            if (!customer)
            { // 必须是登录的 Customer
                cout << "请先以消费者身份登录后再购买商品！" << endl;
                break;
            }
            // --- 开始直接购买逻辑 ---
            string productName_direct;
            int quantity_direct;
            cout << "输入要购买的商品名称: ";
            getline(cin >> ws, productName_direct);

            Product *product_direct = store.findProductByName(productName_direct);
            if (!product_direct)
            {
                cout << "未找到商品 \"" << productName_direct << "\"！" << endl;
                break;
            }

            cout << "输入购买数量: ";
            if (!(cin >> quantity_direct) || quantity_direct <= 0)
            {
                cout << "无效购买数量！" << endl;
                Page::clearInputBuffer();
                break;
            }
            Page::clearInputBuffer();

            double totalCost_direct = product_direct->getPrice() * quantity_direct;
            if (product_direct->getQuantity() < quantity_direct)
            {
                cout << "库存不足！" << endl;
                break;
            }
            if (customer->checkBalance() < totalCost_direct)
            {
                cout << "余额不足！" << endl;
                break;
            }

            if (!customer->withdraw(totalCost_direct))
            { /* cout << "支付失败。" << endl; */
                break;
            } // withdraw 内部会打印

            bool sellerPaymentSuccess_direct = true;
            User *seller_direct = User::findUser(users, product_direct->getSellerUsername());
            if (seller_direct)
            {
                if (!seller_direct->deposit(totalCost_direct))
                {
                    cerr << "错误：向商家 " << seller_direct->getUsername() << " 转账失败。交易回滚。" << endl;
                    customer->deposit(totalCost_direct);
                    sellerPaymentSuccess_direct = false;
                }
            }
            else if (!product_direct->getSellerUsername().empty())
            {
                cerr << "错误：未找到商品 \"" << productName_direct << "\" 的商家 \"" << product_direct->getSellerUsername() << "\"。交易回滚。" << endl;
                customer->deposit(totalCost_direct);
                sellerPaymentSuccess_direct = false;
            }
            else
            {
                cerr << "错误：商品 \"" << productName_direct << "\" 没有商家信息。交易回滚。" << endl;
                customer->deposit(totalCost_direct);
                sellerPaymentSuccess_direct = false;
            }

            if (!sellerPaymentSuccess_direct)
                break;

            product_direct->setQuantity(product_direct->getQuantity() - quantity_direct);
            User::saveUsersToFile(users, USER_FILE);
            store.saveAllProducts(); // 或者 store.saveProductsForSeller(product_direct->getSellerUsername());

            cout << "购买成功！已购买 " << quantity_direct << " 件 \"" << productName_direct << "\"，总花费: " << totalCost_direct << endl;
            // --- 结束直接购买逻辑 ---
            break;
        }
        case 4: // 加入购物车
        {
            if (!customer)
            {
                cout << "请先以消费者身份登录后再使用购物车功能！" << endl;
                break;
            }
            string productName_cart;
            int quantity_to_add;
            cout << "输入要加入购物车的商品名称: ";
            getline(cin >> ws, productName_cart);

            Product *product_to_add = store.findProductByName(productName_cart);
            if (!product_to_add)
            {
                cout << "未找到商品 \"" << productName_cart << "\"！" << endl;
                break;
            }

            cout << "输入数量: ";
            if (!(cin >> quantity_to_add) || quantity_to_add <= 0)
            {
                cout << "无效的数量！" << endl;
                Page::clearInputBuffer();
                break;
            }
            Page::clearInputBuffer();

            customer->addToCart(*product_to_add, quantity_to_add);
            break;
        }
        case 5: // 查看我的购物车
        {
            if (!customer)
            {
                cout << "请先以消费者身份登录后再查看购物车！" << endl;
                break;
            }
            customer->viewCart(store, users); // viewCart 现在是 Customer 的成员函数
            break;
        }
        case 0:
            cout << "正在退出商城..." << endl;
            break;
        default:
            cout << "无效选项。" << endl;
        }
    } while (choice != 0);
    return 1;
}

// 只替换Seller_page::pagestore方法，保留文件其他部分不变

// --- Seller_page 商城实现 ---
int Seller_page::pagestore(User *currentUser, std::vector<User *> &users, Store &store)
{
    // 确保当前用户是商家
    if (!currentUser || currentUser->getUserType() != "seller")
    {
        cout << "错误：只有商家才能访问商家管理页面。" << endl;
        return 1; // 直接退出
    }

    string sellerUsername = currentUser->getUsername();
    int choice = -1;
    do
    {
        cout << "\n--- 商城管理 (商家: " << sellerUsername << ") ---" << endl;
        cout << "1. 显示我的商品" << endl;
        cout << "2. 按名称搜索我的商品" << endl;
        cout << "3. 添加新商品" << endl;
        cout << "4. 修改商品价格" << endl;
        cout << "5. 修改商品库存" << endl;
        cout << "6. 修改商品折扣" << endl;
        cout << "7. 应用分类折扣" << endl;
        // cout << "8. 查看余额" << endl;
        cout << "0. 退出商城管理" << endl;
        cout << "请选择: ";
        if (!(cin >> choice))
        {
            cout << "无效输入。" << endl;
            clearInputBuffer();
            choice = -1;
            continue;
        }
        clearInputBuffer();

        switch (choice)
        {
        case 1:
            store.displaySellerProducts(sellerUsername);
            break;
        case 2:
        {
            string searchTerm;
            cout << "输入搜索名称: ";
            getline(cin >> ws, searchTerm);
            vector<Product *> results = store.searchProductsByName(searchTerm, sellerUsername);
            if (results.empty())
            {
                cout << "未找到匹配商品。" << endl;
            }
            else
            {
                cout << "\n--- 搜索结果 ---" << endl;
                for (const auto *p : results)
                {
                    cout << "-----------------" << endl;
                    p->display();
                }
                cout << "-----------------" << endl;
            }
            break;
        }
        case 3:
        {
            // --- 添加商品逻辑 ---
            int typeChoice;
            cout << "选择商品类型 (1: 图书, 2: 服装, 3: 食品, 4: 其他类型): ";
            if (!(cin >> typeChoice) || typeChoice < 1 || typeChoice > 4)
            {
                cout << "无效类型。" << endl;
                clearInputBuffer();
                break;
            }
            clearInputBuffer();

            string name, desc, spec1, spec2;
            double price;
            int qty;
            cout << "输入商品名称: ";
            getline(cin >> ws, name);
            cout << "输入描述: ";
            getline(cin >> ws, desc);
            cout << "输入价格: ";
            while (!(cin >> price) || price < 0)
            {
                cout << "无效价格\n";
                clearInputBuffer();
                cout << "输入价格: ";
            }
            clearInputBuffer();
            cout << "输入库存数量: ";
            while (!(cin >> qty) || qty < 0)
            {
                cout << "无效数量\n";
                clearInputBuffer();
                cout << "输入库存数量: ";
            }
            clearInputBuffer();

            bool success = false;
            if (typeChoice == 1)
            { // Book
                cout << "输入作者: ";
                getline(cin >> ws, spec1);
                cout << "输入ISBN: ";
                getline(cin >> ws, spec2);
                success = store.createBook(currentUser, name, desc, price, qty, spec1, spec2);
            }
            else if (typeChoice == 2)
            { // Clothing
                cout << "输入尺码: ";
                getline(cin >> ws, spec1);
                cout << "输入颜色: ";
                getline(cin >> ws, spec2);
                success = store.createClothing(currentUser, name, desc, price, qty, spec1, spec2);
            }
            else if (typeChoice == 3)
            { // Food
                cout << "输入过期日期 (YYYY-MM-DD): ";
                getline(cin >> ws, spec1);
                success = store.createFood(currentUser, name, desc, price, qty, spec1);
            }
            else if (typeChoice == 4)
            { // Generic
                cout << "输入分类标签: ";
                getline(cin >> ws, spec1);
                success = store.createGenericProduct(currentUser, name, desc, price, qty, spec1);
            }
            else
            {
                cout << "无效类型。" << endl;
                break;
            }

            if (success)
                cout << "商品 \"" << name << "\" 添加成功！" << endl;
            else
                cout << "商品添加失败！" << endl;
            break;
        }
        case 4:
        { // 修改价格
            string name;
            double price;
            cout << "输入要修改价格的商品名称: ";
            getline(cin >> ws, name);
            cout << "输入新价格: ";
            if (!(cin >> price) || price < 0)
            {
                cout << "无效价格\n";
                clearInputBuffer();
                break;
            }
            clearInputBuffer();
            if (store.manageProductPrice(currentUser, name, price))
                cout << "价格修改成功。\n";
            else
                cout << "价格修改失败。\n";
            break;
        }
        case 5:
        { // 修改库存
            string name;
            int qty;
            cout << "输入要修改库存的商品名称: ";
            getline(cin >> ws, name);
            cout << "输入新库存: ";
            if (!(cin >> qty) || qty < 0)
            {
                cout << "无效库存\n";
                clearInputBuffer();
                break;
            }
            clearInputBuffer();
            if (store.manageProductQuantity(currentUser, name, qty))
                cout << "库存修改成功。\n";
            else
                cout << "库存修改失败。\n";
            break;
        }
        case 6:
        { // 修改折扣
            string name;
            double discount;
            cout << "输入要修改折扣的商品名称: ";
            getline(cin >> ws, name);
            cout << "输入新折扣率 (0 到 100): ";
            if (!(cin >> discount) || discount < 0 || discount > 100)
            {
                cout << "无效折扣率\n";
                clearInputBuffer();
                break;
            }
            clearInputBuffer();
            if (store.manageProductDiscount(currentUser, name, discount / 100))
                cout << "折扣修改成功。\n";
            else
                cout << "折扣修改失败。\n";
            break;
        }
        case 7:
        { // 应用分类折扣
            string category;
            double discount;
            std::vector<std::string> sellerCategories = store.getUniqueCategoriesForSeller(sellerUsername);
            if (sellerCategories.empty())
            {
                cout << "您当前没有任何商品分类可以应用折扣。" << endl;
                break;
            }

            cout << "您当前的商品分类有: ";
            for (size_t i = 0; i < sellerCategories.size(); ++i)
            {
                cout << "\"" << sellerCategories[i] << "\"";
                if (i < sellerCategories.size() - 1)
                {
                    cout << ", ";
                }
            }
            cout << endl;
            cout << "输入要应用折扣的分类名称 (从以上列表选择): ";
            getline(cin >> ws, category);

            bool isValidCategory = false;
            for (const auto &cat : sellerCategories)
            {
                if (cat == category)
                {
                    isValidCategory = true;
                    break;
                }
            }
            if (!isValidCategory)
            {
                cout << "输入的分类 \"" << category << "\" 不是您拥有的有效分类或不存在。" << endl;
                break;
            }

            cout << "输入折扣率 (0 到 100): ";
            if (!(cin >> discount) || discount < 0 || discount > 100)
            {
                cout << "无效折扣率\n";
                clearInputBuffer();
                break;
            }
            clearInputBuffer();
            if (store.applyCategoryDiscount(currentUser, category, discount / 100))
                cout << "分类折扣应用成功。\n";
            else
                cout << "分类折扣应用失败。\n";
            break;
        }
        // case 8:
        // {
        //     double balance = currentUser->checkBalance();
        //     cout << "当前余额: " << balance << endl;
        //     break;
        // }
        case 0:
            cout << "正在退出商城管理..." << endl;
            break;
        default:
            cout << "无效选项。" << endl;
        }
    } while (choice != 0);
    return 1; // 返回1表示正常退出商城，继续主循环
}