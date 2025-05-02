#include "store.h"
#include "../user/user.h" // 需要 User 定义
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm> // 用于 std::find_if

using namespace std;

// --- Product 方法实现 ---
void Product::display() const
{
    std::cout << "  Name: " << name << std::endl;
    std::cout << "  Description: " << description << std::endl;
    std::cout << "  Original Price: $" << std::fixed << std::setprecision(2) << originalPrice << std::endl;
    if (discountRate > 0)
    {
        std::cout << "  Discount: " << (discountRate * 100) << "%" << std::endl;
        std::cout << "  Current Price: $" << std::fixed << std::setprecision(2) << getPrice() << std::endl;
    }
    else
    {
        std::cout << "  Price: $" << std::fixed << std::setprecision(2) << getPrice() << std::endl;
    }
    std::cout << "  Quantity Left: " << quantity << std::endl;
}

void Product::save(std::ofstream &ofs) const
{
    ofs << getType() << "," << name << "," << description << ","
        << originalPrice << "," << quantity << "," << discountRate;
}

void Book::display() const
{
    Product::display();
    std::cout << "  Author: " << author << std::endl;
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
    std::cout << "  Size: " << size << std::endl;
    std::cout << "  Color: " << color << std::endl;
}
void Clothing::save(std::ofstream &ofs) const
{
    Product::save(ofs);
    ofs << "," << size << "," << color;
}

void Food::display() const
{
    Product::display();
    std::cout << "  Expires: " << expirationDate << std::endl;
}
void Food::save(std::ofstream &ofs) const
{
    Product::save(ofs);
    ofs << "," << expirationDate;
}

// --- Store 类实现 ---

Store::Store(const string &filename) : storeFilename(filename)
{
    if (!loadProductsFromFile())
    {
        cerr << "警告: 无法从 " << filename << " 加载商店数据。商店可能为空或文件丢失/损坏。" << endl;
    }
    else
    {
        cout << "已从商店加载 " << products.size() << " 件商品。" << endl;
    }
}

Store::~Store()
{
    for (Product *p : products)
    {
        delete p;
    }
    products.clear();
}

bool Store::loadProductsFromFile()
{
    // ... (加载逻辑保持不变) ...
    ifstream file(storeFilename);
    if (!file.is_open())
        return false;
    for (Product *p : products)
        delete p;
    products.clear();
    string line;
    while (getline(file, line))
    {
        stringstream ss(line);
        string segment;
        vector<string> seglist;
        while (getline(ss, segment, ','))
            seglist.push_back(segment);
        if (seglist.size() < 6)
            continue;
        try
        {
            string type = seglist[0];
            string name = seglist[1];
            string desc = seglist[2];
            double price = stod(seglist[3]);
            int qty = stoi(seglist[4]);
            double discount = stod(seglist[5]);
            Product *newProduct = nullptr;
            if (type == "Book" && seglist.size() >= 8)
                newProduct = new Book(name, desc, price, qty, seglist[6], seglist[7]);
            else if (type == "Clothing" && seglist.size() >= 8)
                newProduct = new Clothing(name, desc, price, qty, seglist[6], seglist[7]);
            else if (type == "Food" && seglist.size() >= 7)
                newProduct = new Food(name, desc, price, qty, seglist[6]);
            if (newProduct)
            {
                newProduct->setDiscountRate(discount);
                products.push_back(newProduct);
            }
        }
        catch (...)
        {
            cerr << "解析行时出错: " << line << endl;
        }
    }
    file.close();
    return true;
}

bool Store::saveProductsToFile()
{
    // ... (保存逻辑保持不变) ...
    ofstream file(storeFilename);
    if (!file.is_open())
    {
        cerr << "错误: 无法打开 " << storeFilename << " 进行保存。" << endl;
        return false;
    }
    for (const Product *p : products)
    {
        p->save(file);
        file << endl;
    }
    file.close();
    cout << "商店数据已保存到 " << storeFilename << "." << endl;
    return true;
}

Product *Store::findProductByName(const string &name)
{
    // ... (查找逻辑保持不变) ...
    auto it = find_if(products.begin(), products.end(),
                      [&name](const Product *p)
                      { return p->getName() == name; });
    return (it != products.end()) ? *it : nullptr;
}

// --- 核心功能实现 ---

void Store::displayAllProducts() const
{
    cout << "\n--- 可用商品 ---" << endl;
    if (products.empty())
    {
        cout << "商店当前为空。" << endl;
        return;
    }
    int count = 0;
    for (const Product *p : products)
    {
        cout << "-------------------------" << endl;
        p->display();
        count++;
    }
    cout << "-------------------------" << endl;
    cout << "共显示商品: " << count << endl;
}

vector<Product *> Store::searchProductsByName(const string &searchTerm) const
{
    vector<Product *> results;
    for (Product *p : products)
    {
        // 简单的包含搜索 (大小写敏感)
        if (p->getName().find(searchTerm) != string::npos)
        {
            results.push_back(p);
        }
    }
    return results;
}

// 添加商品 - 注意：这里传入的是 Product 对象，需要动态复制
// 或者修改为传入所有必要参数来创建
bool Store::addProduct(User *currentUser, const Product &newProductData)
{
    if (!currentUser || currentUser->getUserType() != "商家")
    {
        cerr << "错误: 只有商家可以添加商品。" << endl;
        return false;
    }
    if (findProductByName(newProductData.getName()) != nullptr)
    {
        cerr << "错误: 商品名称 \"" << newProductData.getName() << "\" 已存在。" << endl;
        return false;
    }

    // 需要一种方法来复制传入的 Product 对象，这比较复杂
    // 更好的方法是让 Page 层收集数据，然后调用 Store 的方法来创建
    // 例如: store.createBook(currentUser, name, desc, price, qty, author, isbn);
    // 暂时简化，假设 Page 层已经创建了正确的 Product*
    // 或者修改此函数签名接收 Product*
    cerr << "错误: addProduct(User*, const Product&) 逻辑需要调整以正确创建/复制商品。" << endl;
    return false; // 暂时返回失败

    // --- 替代方案：在 Store 中添加创建方法 ---
    // bool Store::createBook(User* currentUser, string name, ..., string isbn) {
    //     if (!currentUser || currentUser->getUserType() != "商家") return false;
    //     if (findProductByName(name)) return false;
    //     products.push_back(new Book(name, ..., isbn));
    //     return saveProductsToFile();
    // }
    // --- End Alternative ---
}

bool Store::manageProductPrice(User *currentUser, const std::string &productName, double newPrice)
{
    if (!currentUser || currentUser->getUserType() != "商家")
        return false;
    Product *product = findProductByName(productName);
    if (!product || newPrice < 0)
        return false;
    product->setOriginalPrice(newPrice);
    return saveProductsToFile();
}

bool Store::manageProductQuantity(User *currentUser, const std::string &productName, int newQuantity)
{
    if (!currentUser || currentUser->getUserType() != "商家")
        return false;
    Product *product = findProductByName(productName);
    if (!product || newQuantity < 0)
        return false;
    product->setQuantity(newQuantity);
    return saveProductsToFile();
}

bool Store::manageProductDiscount(User *currentUser, const std::string &productName, double newDiscount)
{
    if (!currentUser || currentUser->getUserType() != "商家")
        return false;
    Product *product = findProductByName(productName);
    if (!product || newDiscount < 0.0 || newDiscount > 1.0)
        return false;
    product->setDiscountRate(newDiscount);
    return saveProductsToFile();
}

bool Store::applyCategoryDiscount(User *currentUser, const std::string &category, double discount)
{
    if (!currentUser || currentUser->getUserType() != "商家")
        return false;
    if (discount < 0.0 || discount > 1.0)
        return false;

    bool changed = false;
    for (Product *p : products)
    {
        if (p->getType() == category)
        {
            p->setDiscountRate(discount);
            changed = true;
        }
    }
    if (changed)
        return saveProductsToFile();
    return false; // No products in category or no change needed
}

// 移除了 enterStore, display*Menu, handle*Action

// --- 实现新的创建方法 ---
bool Store::createBook(User *currentUser, const string &name, const string &desc, double price, int qty, const string &author, const string &isbn)
{
    if (!currentUser || currentUser->getUserType() != "商家")
    {
        cerr << "错误: 只有商家可以添加商品。" << endl;
        return false;
    }
    if (findProductByName(name) != nullptr)
    {
        cerr << "错误: 商品名称 \"" << name << "\" 已存在。" << endl;
        return false;
    }
    if (price < 0 || qty < 0)
    {
        cerr << "错误: 价格和数量不能为负数。" << endl;
        return false;
    }
    try
    {
        products.push_back(new Book(name, desc, price, qty, author, isbn));
        return saveProductsToFile();
    }
    catch (const std::bad_alloc &e)
    {
        cerr << "错误: 添加图书时内存分配失败: " << e.what() << endl;
        return false;
    }
    catch (...)
    {
        cerr << "错误: 添加图书时发生未知错误。" << endl;
        return false;
    }
}

bool Store::createClothing(User *currentUser, const string &name, const string &desc, double price, int qty, const string &size, const string &color)
{
    if (!currentUser || currentUser->getUserType() != "商家")
    {
        cerr << "错误: 只有商家可以添加商品。" << endl;
        return false;
    }
    if (findProductByName(name) != nullptr)
    {
        cerr << "错误: 商品名称 \"" << name << "\" 已存在。" << endl;
        return false;
    }
    if (price < 0 || qty < 0)
    {
        cerr << "错误: 价格和数量不能为负数。" << endl;
        return false;
    }
    try
    {
        products.push_back(new Clothing(name, desc, price, qty, size, color));
        return saveProductsToFile();
    }
    catch (const std::bad_alloc &e)
    {
        cerr << "错误: 添加服装时内存分配失败: " << e.what() << endl;
        return false;
    }
    catch (...)
    {
        cerr << "错误: 添加服装时发生未知错误。" << endl;
        return false;
    }
}

bool Store::createFood(User *currentUser, const string &name, const string &desc, double price, int qty, const string &expDate)
{
    if (!currentUser || currentUser->getUserType() != "商家")
    {
        cerr << "错误: 只有商家可以添加商品。" << endl;
        return false;
    }
    if (findProductByName(name) != nullptr)
    {
        cerr << "错误: 商品名称 \"" << name << "\" 已存在。" << endl;
        return false;
    }
    if (price < 0 || qty < 0)
    {
        cerr << "错误: 价格和数量不能为负数。" << endl;
        return false;
    }
    try
    {
        products.push_back(new Food(name, desc, price, qty, expDate));
        return saveProductsToFile();
    }
    catch (const std::bad_alloc &e)
    {
        cerr << "错误: 添加食品时内存分配失败: " << e.what() << endl;
        return false;
    }
    catch (...)
    {
        cerr << "错误: 添加食品时发生未知错误。" << endl;
        return false;
    }
}