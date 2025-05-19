#include "cart.h"
#include "../store/store.h" // For Product and Store definitions
#include "../user/user.h"   // For Customer and User definitions
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm> // For std::remove_if
#include <iomanip>   // For std::fixed, std::setprecision

// Assume USER_FILE is declared extern if needed for saving users after checkout
extern const std::string USER_FILE;

std::string Cart::getCartFilePath() const
{
    return cartDirectoryPath + "/" + ownerUsername + "_cart.txt";
}

Cart::Cart(const std::string &username, const std::string &cartDir)
    : ownerUsername(username), cartDirectoryPath(cartDir)
{
    loadFromFile();
}

Cart::~Cart()
{
    // Optional: Save cart on destruction if there are unsaved changes.
    // However, explicit save calls after modifications are generally safer.
    // saveToFile();
}

void Cart::loadFromFile()
{
    items.clear();
    std::string cartPath = getCartFilePath();
    std::ifstream file(cartPath);

    if (!file.is_open())
    {
        return; // No cart file, or cannot open; cart remains empty
    }

    std::string line;
    while (getline(file, line))
    {
        std::stringstream ss(line);
        std::string productId, productName, sellerUsernameStr;
        int quantity;
        double priceAtAddition;
        if (getline(ss, productId, ',') &&
            getline(ss, productName, ',') &&
            (ss >> quantity) &&
            (ss.ignore(1) && ss >> priceAtAddition) &&
            (ss.ignore(1) && getline(ss, sellerUsernameStr)))
        {
            items.emplace_back(productId, productName, quantity, priceAtAddition, sellerUsernameStr);
        }
        else
        {
            std::cerr << "警告: 购物车文件 " << cartPath << " 中存在格式错误的行: " << line << std::endl;
        }
    }
    file.close();
}

bool Cart::saveToFile() const
{
    if (ownerUsername.empty())
    {
        std::cerr << "错误: 购物车缺少所有者信息，无法保存。" << std::endl;
        return false;
    }
    std::string cartPath = getCartFilePath();
    std::ofstream file(cartPath);

    if (!file.is_open())
    {
        std::cerr << "错误: 无法打开购物车文件进行写入: " << cartPath << std::endl;
        return false;
    }

    for (const auto &item : items)
    {
        file << item.productId << ","
             << item.productName << ","
             << item.quantity << ","
             << item.priceAtAddition << ","
             << item.sellerUsername << std::endl;
    }
    file.close();
    return true;
}

bool Cart::addItem(const Product &product, int quantity)
{
    if (quantity <= 0)
    {
        std::cout << "添加数量必须大于0。" << std::endl;
        return false;
    }

    bool found = false;
    for (auto &item : items)
    {
        if (item.productId == product.getName())
        { // Assuming product name is unique ID
            item.quantity += quantity;
            // item.priceAtAddition = product.getPrice(); // Optional: update price to current
            found = true;
            break;
        }
    }

    if (!found)
    {
        items.emplace_back(product.getName(), product.getName(), quantity, product.getPrice(), product.getSellerUsername());
    }
    std::cout << "\"" << product.getName() << "\" 已成功加入购物车。" << std::endl;
    return saveToFile();
}

bool Cart::removeItem(const std::string &productName)
{
    auto initial_size = items.size();
    items.erase(std::remove_if(items.begin(), items.end(),
                               [&](const CartItem &item)
                               { return item.productName == productName; }),
                items.end());

    if (items.size() < initial_size)
    {
        std::cout << "商品 \"" << productName << "\" 已从购物车移除。" << std::endl;
        return saveToFile();
    }
    else
    {
        std::cout << "未在购物车中找到商品: " << productName << std::endl;
        return false;
    }
}

bool Cart::updateItemQuantity(const std::string &productName, int newQuantity, Store &store)
{
    if (newQuantity < 0)
    {
        std::cout << "数量不能为负。" << std::endl;
        return false;
    }

    bool updated = false;
    for (auto &item : items)
    {
        if (item.productName == productName)
        {
            Product *p_info = store.findProductByName(productName);
            if (p_info && newQuantity > p_info->getQuantity())
            {
                std::cout << "库存不足！无法将购物车中 \"" << productName << "\" 的数量修改为 " << newQuantity
                          << "。当前库存: " << p_info->getQuantity() << std::endl;
                return false; // Do not update if stock is insufficient
            }
            item.quantity = newQuantity;
            updated = true;
            break;
        }
    }

    if (!updated)
    {
        std::cout << "未在购物车中找到商品: " << productName << std::endl;
        return false;
    }

    // Remove items with zero quantity
    items.erase(std::remove_if(items.begin(), items.end(),
                               [](const CartItem &item)
                               { return item.quantity <= 0; }),
                items.end());

    std::cout << "购物车商品 \"" << productName << "\" 数量已更新。" << std::endl;
    return saveToFile();
}

void Cart::displayCart(Store &store) const
{
    if (items.empty())
    {
        std::cout << "\n您的购物车是空的。" << std::endl;
        return;
    }

    double totalCartPriceBasedOnCurrent = 0;
    std::cout << "\n--- 我的购物车 ---" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    for (size_t i = 0; i < items.size(); ++i)
    {
        const auto &item = items[i];
        Product *p_info = store.findProductByName(item.productId);
        double currentItemPrice = p_info ? p_info->getPrice() : item.priceAtAddition;

        std::cout << i + 1 << ". 商品: " << item.productName
                  << " | 数量: " << item.quantity
                  << " | 当前单价: ¥" << currentItemPrice
                  << " | 小计: ¥" << (item.quantity * currentItemPrice) << std::endl;
        if (p_info)
        {
            if (p_info->getQuantity() < item.quantity)
            {
                std::cout << "   注意: " << item.productName << " 当前库存(" << p_info->getQuantity() << ")不足购物车数量(" << item.quantity << ")!" << std::endl;
            }
            if (p_info->getPrice() != item.priceAtAddition)
            {
                std::cout << "   提示: " << item.productName << " 加入时价格为 ¥" << item.priceAtAddition << ", 当前价格已变为 ¥" << p_info->getPrice() << std::endl;
            }
        }
        else
        {
            std::cout << "   警告: 商品 " << item.productName << " 可能已从商店下架!" << std::endl;
        }
        totalCartPriceBasedOnCurrent += item.quantity * currentItemPrice;
    }
    std::cout << "--------------------" << std::endl;
    std::cout << "购物车总计 (按当前价格): ¥" << totalCartPriceBasedOnCurrent << std::endl;
    std::cout << "--------------------" << std::endl;
    std::cout << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
}

bool Cart::checkout(Customer *buyer, Store &store, std::vector<User *> &allUsers)
{
    if (!buyer)
    {
        std::cout << "错误: 无效的购买者信息。" << std::endl;
        return false;
    }
    if (items.empty())
    {
        std::cout << "购物车是空的，无法结算。" << std::endl;
        return false;
    }

    double finalTotalCost = 0;
    bool canCheckout = true;
    std::vector<std::pair<Product *, int>> productsForTransaction;

    // 1. Validate items and calculate total cost based on current store prices
    for (const auto &item : items)
    {
        Product *product = store.findProductByName(item.productId);
        if (!product)
        {
            std::cout << "错误: 购物车中的商品 \"" << item.productName << "\" 已不存在或无法找到。" << std::endl;
            canCheckout = false;
            break;
        }
        if (product->getQuantity() < item.quantity)
        {
            std::cout << "错误: 商品 \"" << item.productName << "\" 库存不足 (需要 "
                      << item.quantity << ", 现有 " << product->getQuantity() << ")。" << std::endl;
            canCheckout = false;
            break;
        }
        finalTotalCost += product->getPrice() * item.quantity;
        productsForTransaction.push_back({product, item.quantity});
    }

    if (!canCheckout)
        return false;

    // 2. Check buyer's balance
    if (buyer->checkBalance() < finalTotalCost)
    {
        std::cout << "余额不足！当前余额: " << buyer->checkBalance()
                  << "，需要: " << finalTotalCost << std::endl;
        return false;
    }

    // 3. Perform transaction: Deduct buyer's balance
    if (!buyer->withdraw(finalTotalCost))
    {
        std::cout << "支付失败，无法完成结算。" << std::endl;
        return false; // Withdraw failed
    }

    // 4. Update product stock and pay sellers
    for (const auto &trans_item : productsForTransaction)
    {
        Product *product = trans_item.first;
        int quantityBought = trans_item.second;

        product->setQuantity(product->getQuantity() - quantityBought);

        if (!product->getSellerUsername().empty())
        {
            User *seller = User::findUser(allUsers, product->getSellerUsername());
            if (seller)
            {
                if (!seller->deposit(product->getPrice() * quantityBought))
                {
                    std::cerr << "警告: 向商家 " << seller->getUsername() << " 转账失败 (商品: " << product->getName() << ")" << std::endl;
                }
            }
            else
            {
                std::cerr << "警告: 未找到商品 \"" << product->getName() << "\" 的商家 \"" << product->getSellerUsername() << "\"，款项可能未到账。" << std::endl;
            }
        }
        else
        {
            std::cerr << "警告: 商品 \"" << product->getName() << "\" 没有商家信息。" << std::endl;
        }
    }

    // 5. Save all changes
    User::saveUsersToFile(allUsers, USER_FILE); // Assumes USER_FILE is accessible
    store.saveAllProducts();

    // 6. Clear the cart (in memory and file)
    clearCartAndFile();

    std::cout << "购物车已成功结算！总花费: ¥" << std::fixed << std::setprecision(2) << finalTotalCost << std::endl;
    std::cout << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
    return true;
}

bool Cart::isEmpty() const
{
    return items.empty();
}

void Cart::clearCartAndFile()
{
    items.clear();
    std::string cartPath = getCartFilePath();
    if (std::filesystem::exists(cartPath))
    {
        try
        {
            std::filesystem::remove(cartPath);
        }
        catch (const std::filesystem::filesystem_error &err)
        {
            std::cerr << "错误: 删除购物车文件失败: " << err.what() << std::endl;
        }
    }
}
