#include "store.h"
#include "../user/user.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <set>
#include <direct.h> // Windows 系统特定的目录操作

namespace fs = std::filesystem;

using namespace std;

// --- Product 方法实现 ---
void Product::display() const
{
    std::cout << "  商户名称：" << sellerUsername << std::endl;
    std::cout << "  商品名称: " << name << std::endl;
    std::cout << "  描述: " << description << std::endl;

    if (!getUserCategory().empty())
    {
        std::cout << "  分类标签: " << getUserCategory() << std::endl;
    }

    if (discountRate > 0)
    {
        std::cout << "  原价: ¥" << std::fixed << std::setprecision(2) << originalPrice << std::endl;
        std::cout << "  折扣: " << (discountRate * 100) << "%" << std::endl;
        std::cout << "  现价: ¥" << std::fixed << std::setprecision(2) << getPrice() << std::endl;
    }
    else
    {
        std::cout << "  价格: ¥" << std::fixed << std::setprecision(2) << getPrice() << std::endl;
    }
    std::cout << "  库存: " << quantity << " 件" << std::endl;
    if (!sellerUsername.empty())
    {
        std::cout << "  商家: " << sellerUsername << std::endl;
    }
}

void Product::save(std::ofstream &ofs) const
{
    ofs << getType() << "," << name << "," << description << ","
        << originalPrice << "," << quantity << "," << discountRate << "," << sellerUsername;
}

void Book::display() const // 复用父类
{
    Product::display();
    std::cout << "  作者: " << author << std::endl;
    std::cout << "  ISBN: " << isbn << std::endl;
}

void Book::save(std::ofstream &ofs) const
{
    Product::save(ofs);
    ofs << "," << author << "," << isbn;
}

void Clothing::display() const
{
    Product::display();
    std::cout << "  尺寸: " << size << std::endl;
    std::cout << "  颜色: " << color << std::endl;
}

void Clothing::save(std::ofstream &ofs) const
{
    Product::save(ofs);
    ofs << "," << size << "," << color;
}

void Food::display() const
{
    Product::display();
    std::cout << "  保质期限: " << expirationDate << std::endl;
}

void Food::save(std::ofstream &ofs) const
{
    Product::save(ofs);
    ofs << "," << expirationDate;
}

void GenericProduct::display() const
{
    Product::display();
}

void GenericProduct::save(std::ofstream &ofs) const
{
    Product::save(ofs);
    ofs << "," << categoryTag;
}

// --- Store 类实现 ---

// 构造函数
Store::Store(const string &directory) : storeDirectory(directory)
{
    // 确保商家目录存在
    string sellersDir = storeDirectory + "/sellers";
    ensureDirectoryExists(sellersDir);

    // 加载所有商品数据
    if (!loadAllProducts())
    {
        cerr << "警告: 无法加载商店数据。商店可能为空或文件丢失/损坏。" << endl;
    }
    else
    {
        cout << "已从商店加载 " << allProducts.size() << " 件商品。" << endl;
    }
}

// 析构函数
Store::~Store()
{
    for (Product *p : allProducts)
    {
        delete p;
    }
    allProducts.clear();
    sellerProducts.clear(); // 清除映射
}

// 确保目录存在
bool Store::ensureDirectoryExists(const string &path) const
{
    try
    {
        if (!fs::exists(path))
        {
            fs::create_directories(path);
            cout << "创建目录: " << path << endl;
        }
        return true;
    }
    catch (const fs::filesystem_error &e)
    {
        cerr << "创建目录失败 " << path << ": " << e.what() << endl;

        // 尝试使用传统方法创建目录
        if (_mkdir(path.c_str()) == 0)
        {
            cout << "使用传统方法创建目录: " << path << endl;
            return true;
        }

        return false;
    }
    catch (...)
    {
        cerr << "创建目录时发生未知错误: " << path << endl;
        return false;
    }
}

// 获取商家文件名
string Store::getSellerFilename(const string &username) const
{
    return storeDirectory + "/sellers/" + username + ".txt";
}

// 加载所有商品
bool Store::loadAllProducts()
{
    // 清空现有商品
    for (Product *p : allProducts)
    {
        delete p;
    }
    allProducts.clear();
    sellerProducts.clear();

    // 获取sellers目录下的所有文件
    string sellersDir = storeDirectory + "/sellers";

    try
    {
        // 如果目录不存在，创建目录并返回成功（表示商店为空）
        if (!fs::exists(sellersDir))
        {
            ensureDirectoryExists(sellersDir);
            return true;
        }

        // 遍历目录中的所有文件
        for (const auto &entry : fs::directory_iterator(sellersDir))
        {
            if (entry.is_regular_file())
            {
                string filename = entry.path().string();
                string sellerUsername = entry.path().stem().string(); // 获取不带扩展名的文件名

                loadSellerProducts(sellerUsername);
            }
        }
        return true;
    }
    catch (const fs::filesystem_error &e)
    {
        cerr << "读取商家目录失败: " << e.what() << endl;
        return false;
    }
    catch (...)
    {
        cerr << "读取商家数据时发生未知错误" << endl;
        return false;
    }
}

// 加载指定商家的商品
bool Store::loadSellerProducts(const string &sellerUsername)
{
    string filename = getSellerFilename(sellerUsername);
    ifstream file(filename);
    if (!file.is_open())
    {
        // 对于新商家，文件不存在是正常的
        return true;
    }

    vector<Product *> sellerProductsList;
    string line;

    while (getline(file, line))
    {
        stringstream ss(line);
        string segment;
        vector<string> seglist;

        while (getline(ss, segment, ','))
        {
            seglist.push_back(segment);
        }

        // 格式检查: 至少需要7个字段 (类型,名称,描述,价格,数量,折扣,商家)
        if (seglist.size() < 7)
        {
            cerr << "无效的产品数据行: " << line << endl;
            continue;
        }

        try
        {
            string type = seglist[0];
            string name = seglist[1];
            string desc = seglist[2];
            double price = stod(seglist[3]);
            int qty = stoi(seglist[4]);
            double discount = stod(seglist[5]);
            string seller = seglist[6]; // 第7个字段是商家名

            // 确认商家名称匹配
            if (seller != sellerUsername)
            {
                cerr << "警告: 文件 " << filename << " 中的商品所属商家 \""
                     << seller << "\" 与预期的 \"" << sellerUsername << "\" 不符" << endl;
                seller = sellerUsername; // 强制使用文件名指示的商家
            }

            Product *newProduct = nullptr;

            if (type == "Book" && seglist.size() >= 9)
            {
                // Book需要额外的author和isbn字段
                newProduct = new Book(name, desc, price, qty, seglist[7], seglist[8], seller);
            }
            else if (type == "Clothing" && seglist.size() >= 9)
            {
                // Clothing需要额外的size和color字段
                newProduct = new Clothing(name, desc, price, qty, seglist[7], seglist[8], seller);
            }
            else if (type == "Food" && seglist.size() >= 8)
            {
                // Food需要额外的expirationDate字段
                newProduct = new Food(name, desc, price, qty, seglist[7], seller);
            }
            else if (type == "Generic" && seglist.size() >= 8)
            {
                // GenericProduct需要额外的categoryTag字段
                newProduct = new GenericProduct(name, desc, price, qty, seglist[7], seller);
            }
            else
            {
                cerr << "未知商品类型或缺少必要字段: " << line << endl;
                continue;
            }

            if (newProduct)
            {
                newProduct->setDiscountRate(discount);
                allProducts.push_back(newProduct);
                sellerProductsList.push_back(newProduct);
            }
        }
        catch (const exception &e)
        {
            cerr << "解析商品数据时出错: " << e.what() << " 行: " << line << endl;
        }
    }

    file.close();

    // 更新商家商品映射
    sellerProducts[sellerUsername] = sellerProductsList;
    cout << "已加载商家 \"" << sellerUsername << "\" 的 "
         << sellerProductsList.size() << " 件商品" << endl;

    return true;
}

// 保存所有商品
bool Store::saveAllProducts()
{
    // 对于每个商家，保存其商品
    set<string> processedSellers; // 用于跟踪已处理的商家

    for (const auto &product : allProducts)
    {
        string seller = product->getSellerUsername();
        if (!seller.empty() && processedSellers.find(seller) == processedSellers.end())
        {
            saveProductsForSeller(seller);
            processedSellers.insert(seller);
        }
    }

    return true;
}

// 保存指定商家的商品
bool Store::saveProductsForSeller(const string &sellerUsername)
{
    if (sellerUsername.empty())
    {
        cerr << "错误: 尝试保存无效商家的商品" << endl;
        return false;
    }

    vector<Product *> products;

    // 获取该商家的所有商品
    for (Product *p : allProducts)
    {
        if (p->getSellerUsername() == sellerUsername)
        {
            products.push_back(p);
        }
    }

    string filename = getSellerFilename(sellerUsername);
    ofstream file(filename);

    if (!file.is_open())
    {
        cerr << "错误: 无法打开文件保存商品: " << filename << endl;
        return false;
    }

    for (const Product *p : products)
    {
        p->save(file);
        file << endl;
    }

    file.close();
    // cout << "商家 \"" << sellerUsername << "\" 的 " << products.size()
    //      << " 件商品已保存至 " << filename << endl;

    return true;
}

// 查找商品
Product *Store::findProductByName(const string &name, const string &sellerUsername) const
{
    if (!sellerUsername.empty())
    {
        // 只在指定商家的商品中查找
        auto it = sellerProducts.find(sellerUsername);
        if (it != sellerProducts.end())
        {
            for (Product *p : it->second)
            {
                if (p->getName() == name)
                {
                    return p;
                }
            }
        }
        return nullptr;
    }
    else
    {
        // 在所有商品中查找
        for (Product *p : allProducts)
        {
            if (p->getName() == name)
            {
                return p;
            }
        }
        return nullptr;
    }
}

// 显示所有商品
void Store::displayAllProducts() const
{
    if (allProducts.empty())
    {
        cout << "\n--- 商品列表为空 ---" << endl;
        return;
    }

    cout << "\n--- 所有商品列表 ---" << endl;
    int count = 0;

    for (const Product *p : allProducts)
    {
        cout << "--------------------------" << endl;
        p->display();
        count++;
    }

    cout << "--------------------------" << endl;
    cout << "共显示 " << count << " 件商品" << endl;
}

// 显示指定商家的商品
void Store::displaySellerProducts(const string &sellerUsername) const
{
    auto it = sellerProducts.find(sellerUsername);

    if (it == sellerProducts.end() || it->second.empty())
    {
        cout << "\n--- 商家 \"" << sellerUsername << "\" 没有商品 ---" << endl;
        return;
    }

    cout << "\n--- 商家 \"" << sellerUsername << "\" 的商品列表 ---" << endl;
    int count = 0;

    for (const Product *p : it->second)
    {
        cout << "--------------------------" << endl;
        p->display();
        count++;
    }

    cout << "--------------------------" << endl;
    cout << "共显示 " << count << " 件商品" << endl;
}

// 按名称搜索商品
vector<Product *> Store::searchProductsByName(const string &searchTerm, const string &sellerUsername) const
{
    vector<Product *> results;

    const vector<Product *> &productsToSearch = sellerUsername.empty() ? allProducts : (sellerProducts.count(sellerUsername) ? sellerProducts.at(sellerUsername) : vector<Product *>());

    for (Product *p : productsToSearch)
    {
        // 简单的包含搜索 (不区分大小写)
        string productName = p->getName();
        string searchTermLower = searchTerm;

        // 将两个字符串转为小写进行比较
        transform(productName.begin(), productName.end(), productName.begin(), ::tolower);
        transform(searchTermLower.begin(), searchTermLower.end(), searchTermLower.begin(), ::tolower);

        if (productName.find(searchTermLower) != string::npos)
        {
            results.push_back(p);
        }
    }

    return results;
}

// 获取商家的商品
vector<Product *> Store::getSellerProducts(const string &sellerUsername) const
{
    auto it = sellerProducts.find(sellerUsername);
    return (it != sellerProducts.end()) ? it->second : vector<Product *>();
}

// 创建图书
bool Store::createBook(User *currentUser, const string &name, const string &desc,
                       double price, int qty, const string &author, const string &isbn)
{
    if (!currentUser)
    {
        cerr << "错误: 用户未登录" << endl;
        return false;
    }

    if (currentUser->getUserType() != "seller")
    {
        cerr << "错误: 只有商家可以添加商品" << endl;
        return false;
    }

    string sellerUsername = currentUser->getUsername();

    // 检查该商家是否已有同名商品
    if (findProductByName(name, sellerUsername) != nullptr)
    {
        cerr << "错误: 您已有名为 \"" << name << "\" 的商品" << endl;
        return false;
    }

    if (price < 0 || qty < 0)
    {
        cerr << "错误: 价格和数量不能为负数" << endl;
        return false;
    }

    try
    {
        Book *newBook = new Book(name, desc, price, qty, author, isbn, sellerUsername);
        allProducts.push_back(newBook);

        // 更新商家商品映射
        sellerProducts[sellerUsername].push_back(newBook);

        // 保存商家的商品
        if (saveProductsForSeller(sellerUsername))
        {
            cout << "商品 \"" << name << "\" 添加成功！" << endl;
            return true;
        }
        else
        {
            cout << "商品 \"" << name << "\" 添加失败！" << endl;
            return false;
        }
    }
    catch (const bad_alloc &e)
    {
        cerr << "错误: 无法分配内存: " << e.what() << endl;
        return false;
    }
    catch (const exception &e)
    {
        cerr << "错误: " << e.what() << endl;
        return false;
    }
    catch (...)
    {
        cerr << "错误: 添加商品时发生未知错误" << endl;
        return false;
    }
}

// 创建服装
bool Store::createClothing(User *currentUser, const string &name, const string &desc,
                           double price, int qty, const string &size, const string &color)
{
    if (!currentUser)
    {
        cerr << "错误: 用户未登录" << endl;
        return false;
    }

    if (currentUser->getUserType() != "seller")
    {
        cerr << "错误: 只有商家可以添加商品" << endl;
        return false;
    }

    string sellerUsername = currentUser->getUsername();

    // 检查该商家是否已有同名商品
    if (findProductByName(name, sellerUsername) != nullptr)
    {
        cerr << "错误: 您已有名为 \"" << name << "\" 的商品" << endl;
        return false;
    }

    if (price < 0 || qty < 0)
    {
        cerr << "错误: 价格和数量不能为负数" << endl;
        return false;
    }

    try
    {
        Clothing *newClothing = new Clothing(name, desc, price, qty, size, color, sellerUsername);
        allProducts.push_back(newClothing);

        // 更新商家商品映射
        sellerProducts[sellerUsername].push_back(newClothing);

        // 保存商家的商品

        if (saveProductsForSeller(sellerUsername))
        {
            cout << "商品 \"" << name << "\" 添加成功！" << endl;
            return true;
        }
        else
        {
            cout << "商品 \"" << name << "\" 添加失败！" << endl;
            return false;
        }
    }
    catch (const bad_alloc &e)
    {
        cerr << "错误: 无法分配内存: " << e.what() << endl;
        return false;
    }
    catch (const exception &e)
    {
        cerr << "错误: " << e.what() << endl;
        return false;
    }
    catch (...)
    {
        cerr << "错误: 添加商品时发生未知错误" << endl;
        return false;
    }
}

// 创建食品
bool Store::createFood(User *currentUser, const string &name, const string &desc,
                       double price, int qty, const string &expDate)
{
    if (!currentUser)
    {
        cerr << "错误: 用户未登录" << endl;
        return false;
    }

    if (currentUser->getUserType() != "seller")
    {
        cerr << "错误: 只有商家可以添加商品" << endl;
        return false;
    }

    string sellerUsername = currentUser->getUsername();

    // 检查该商家是否已有同名商品
    if (findProductByName(name, sellerUsername) != nullptr)
    {
        cerr << "错误: 您已有名为 \"" << name << "\" 的商品" << endl;
        return false;
    }

    if (price < 0 || qty < 0)
    {
        cerr << "错误: 价格和数量不能为负数" << endl;
        return false;
    }

    try
    {
        Food *newFood = new Food(name, desc, price, qty, expDate, sellerUsername);
        allProducts.push_back(newFood);

        // 更新商家商品映射
        sellerProducts[sellerUsername].push_back(newFood);

        // 保存商家的商品
        if (saveProductsForSeller(sellerUsername))
        {
            cout << "商品 \"" << name << "\" 添加成功！" << endl;
            return true;
        }
        else
        {
            cout << "商品 \"" << name << "\" 添加失败！" << endl;
            return false;
        }
    }
    catch (const bad_alloc &e)
    {
        cerr << "错误: 无法分配内存: " << e.what() << endl;
        return false;
    }
    catch (const exception &e)
    {
        cerr << "错误: " << e.what() << endl;
        return false;
    }
    catch (...)
    {
        cerr << "错误: 添加商品时发生未知错误" << endl;
        return false;
    }
}

bool Store::createGenericProduct(User *currentUser, const string &name, const string &desc,
                                 double price, int qty, const string &categoryTag)
{
    if (!currentUser)
    {
        cerr << "错误: 用户未登录" << endl;
        return false;
    }

    if (currentUser->getUserType() != "seller")
    {
        cerr << "错误: 只有商家可以添加商品" << endl;
        return false;
    }

    string sellerUsername = currentUser->getUsername();

    if (findProductByName(name, sellerUsername) != nullptr)
    {
        cerr << "错误: 您已有名为 \"" << name << "\" 的商品" << endl;
        return false;
    }

    if (price < 0 || qty < 0)
    {
        cerr << "错误: 价格和数量不能为负数" << endl;
        return false;
    }
    if (categoryTag.empty())
    {
        cerr << "错误: 通用商品必须指定分类标签" << endl;
        return false;
    }

    try
    {
        GenericProduct *newGenericProduct = new GenericProduct(name, desc, price, qty, categoryTag, sellerUsername);
        allProducts.push_back(newGenericProduct);
        sellerProducts[sellerUsername].push_back(newGenericProduct);

        if (saveProductsForSeller(sellerUsername))
        {
            cout << "通用商品 \"" << name << "\" (分类: " << categoryTag << ") 添加成功！" << endl;
            return true;
        }
        else
        {
            // 如果保存失败，应该从内存中移除，以保持一致性
            allProducts.pop_back();
            sellerProducts[sellerUsername].pop_back();
            delete newGenericProduct;
            cout << "通用商品 \"" << name << "\" 添加失败（保存错误）！" << endl;
            return false;
        }
    }
    catch (const bad_alloc &e)
    {
        cerr << "错误: 无法分配内存: " << e.what() << endl;
        return false;
    }
    catch (const exception &e)
    {
        cerr << "错误: " << e.what() << endl;
        return false;
    }
    catch (...)
    {
        cerr << "错误: 添加通用商品时发生未知错误" << endl;
        return false;
    }
}

// 修改商品价格
bool Store::manageProductPrice(User *currentUser, const string &productName, double newPrice)
{
    if (!currentUser)
    {
        cerr << "错误: 用户未登录" << endl;
        return false;
    }

    if (currentUser->getUserType() != "seller")
    {
        cerr << "错误: 只有商家可以修改商品价格" << endl;
        return false;
    }

    string sellerUsername = currentUser->getUsername();
    Product *product = findProductByName(productName, sellerUsername);

    if (!product)
    {
        cerr << "错误: 未找到您的商品 \"" << productName << "\"" << endl;
        return false;
    }

    if (newPrice < 0)
    {
        cerr << "错误: 价格不能为负数" << endl;
        return false;
    }

    product->setOriginalPrice(newPrice);
    // 保存商家的商品
    if (saveProductsForSeller(sellerUsername))
    {
        cout << "价格保存成功！" << endl;
        return true;
    }
    else
    {
        cout << "价格保存失败！" << endl;
        return false;
    }
}

// 修改商品库存
bool Store::manageProductQuantity(User *currentUser, const string &productName, int newQuantity)
{
    if (!currentUser)
    {
        cerr << "错误: 用户未登录" << endl;
        return false;
    }

    if (currentUser->getUserType() != "seller")
    {
        cerr << "错误: 只有商家可以修改商品库存" << endl;
        return false;
    }

    string sellerUsername = currentUser->getUsername();
    Product *product = findProductByName(productName, sellerUsername);

    if (!product)
    {
        cerr << "错误: 未找到您的商品 \"" << productName << "\"" << endl;
        return false;
    }

    if (newQuantity < 0)
    {
        cerr << "错误: 库存不能为负数" << endl;
        return false;
    }

    product->setQuantity(newQuantity);
    if (saveProductsForSeller(sellerUsername))
    {
        cout << "库存修改成功！" << endl;
        return true;
    }
    else
    {
        cout << "库存修改失败！" << endl;
        return false;
    }
}

// 修改商品折扣
bool Store::manageProductDiscount(User *currentUser, const string &productName, double newDiscount)
{
    if (!currentUser)
    {
        cerr << "错误: 用户未登录" << endl;
        return false;
    }

    if (currentUser->getUserType() != "seller")
    {
        cerr << "错误: 只有商家可以修改商品折扣" << endl;
        return false;
    }

    string sellerUsername = currentUser->getUsername();
    Product *product = findProductByName(productName, sellerUsername);

    if (!product)
    {
        cerr << "错误: 未找到您的商品 \"" << productName << "\"" << endl;
        return false;
    }

    if (newDiscount < 0 || newDiscount > 100)
    {
        cerr << "错误: 折扣必须在0到100之间" << endl;
        return false;
    }

    product->setDiscountRate(newDiscount);
    return saveProductsForSeller(sellerUsername);
}

std::vector<std::string> Store::getUniqueCategoriesForSeller(const std::string &sellerUsername) const
{
    std::set<std::string> uniqueCategories;
    auto it = sellerProducts.find(sellerUsername);
    if (it != sellerProducts.end())
    {
        for (const Product *p : it->second)
        {
            if (p)
            {                                                // Ensure product pointer is not null
                std::string category = p->getUserCategory(); // This gets "Book", "Clothing", "Food", or custom tag
                if (!category.empty())
                { // Ensure not to add empty strings
                    uniqueCategories.insert(category);
                }
            }
        }
    }
    return std::vector<std::string>(uniqueCategories.begin(), uniqueCategories.end());
}

// 应用分类折扣
bool Store::applyCategoryDiscount(User *currentUser, const string &category, double discount)
{
    if (!currentUser)
    {
        cerr << "错误: 用户未登录" << endl;
        return false;
    }

    if (currentUser->getUserType() != "seller")
    {
        cerr << "错误: 只有商家可以应用折扣" << endl;
        return false;
    }

    if (discount < 0.0 || discount > 1.0)
    {
        cerr << "错误: 折扣必须在0到100之间" << endl;
        return false;
    }

    string sellerUsername = currentUser->getUsername();
    bool changed = false;

    // 只应用折扣到该商家的商品
    auto it = sellerProducts.find(sellerUsername);
    if (it != sellerProducts.end())
    {
        for (Product *p : it->second)
        {
            // 使用 getUserCategory() 进行比较
            if (p && p->getUserCategory() == category)
            {
                p->setDiscountRate(discount);
                changed = true;
            }
        }
    }

    if (changed)
    {
        cout << "已对分类 \"" << category << "\" 下的商品应用 " << (discount * 100) << "% 折扣。" << endl;
        return saveProductsForSeller(sellerUsername);
    }

    cerr << "错误: 未找到分类为 \"" << category << "\" 的商品" << endl;
    return false;
}
