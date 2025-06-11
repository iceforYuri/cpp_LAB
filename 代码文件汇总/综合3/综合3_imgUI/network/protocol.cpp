#include "protocol.h"
#include <sstream>
#include <iostream>

namespace Protocol
{

    // Message序列化实现
    std::string Message::serialize() const
    {
        std::ostringstream oss;
        oss << static_cast<int>(type) << "|" << sessionId << "|";

        // 序列化数据字段
        oss << data.size() << "|";
        for (const auto &pair : data)
        {
            oss << pair.first << "=" << pair.second << "|";
        }

        return oss.str();
    }
    Message Message::deserialize(const std::string &str)
    {
        Message msg;
        size_t pos = 0;
        std::string token;

        try
        {
            // 解析消息类型
            size_t next_pos = str.find('|', pos);
            if (next_pos != std::string::npos)
            {
                token = str.substr(pos, next_pos - pos);
                msg.type = static_cast<MessageType>(std::stoi(token));
                pos = next_pos + 1;
            }

            // 解析会话ID
            next_pos = str.find('|', pos);
            if (next_pos != std::string::npos)
            {
                msg.sessionId = str.substr(pos, next_pos - pos);
                pos = next_pos + 1;
            }

            // 解析数据字段数量
            int dataCount = 0;
            next_pos = str.find('|', pos);
            if (next_pos != std::string::npos)
            {
                token = str.substr(pos, next_pos - pos);
                dataCount = std::stoi(token);
                pos = next_pos + 1;
            }

            // 解析数据字段 - 改进的逻辑
            int parsedCount = 0;
            while (parsedCount < dataCount && pos < str.length())
            {
                // 查找下一个 key=value 对
                next_pos = str.find('|', pos);
                if (next_pos == std::string::npos)
                {
                    // 最后一个字段
                    token = str.substr(pos);
                }
                else
                {
                    token = str.substr(pos, next_pos - pos);
                }

                // 解析 key=value
                size_t eq_pos = token.find('=');
                if (eq_pos != std::string::npos)
                {
                    std::string key = token.substr(0, eq_pos);
                    std::string value = token.substr(eq_pos + 1); // 对于包含内部分隔符的值（如订单数据），需要特殊处理
                    if (key.find("order_") == 0)
                    {
                        // 这是订单数据，可能包含内部的 | 分隔符
                        // 需要查找到下一个 key= 模式来确定真正的结束位置
                        size_t search_pos = next_pos + 1;
                        while (search_pos < str.length())
                        {
                            size_t temp_pos = str.find('|', search_pos);
                            if (temp_pos == std::string::npos)
                            {
                                // 没有更多的 | 分隔符，包含剩余所有内容
                                if (search_pos < str.length())
                                {
                                    value += "|" + str.substr(search_pos);
                                }
                                next_pos = std::string::npos; // 标记为最后一个字段
                                break;
                            }

                            std::string temp_token = str.substr(search_pos, temp_pos - search_pos);
                            if (temp_token.find('=') != std::string::npos &&
                                (temp_token.find("order_") == 0 || temp_token == "count" || temp_token.find("user_") == 0))
                            {
                                // 找到了下一个有效的 key=value 对
                                break;
                            }

                            // 继续包含到值中
                            value += "|" + temp_token;
                            next_pos = temp_pos;
                            search_pos = temp_pos + 1;
                        }
                    }

                    msg.data[key] = value;
                }

                parsedCount++;
                if (next_pos != std::string::npos)
                {
                    pos = next_pos + 1;
                }
                else
                {
                    break;
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "消息反序列化错误: " << e.what() << std::endl;
        }

        return msg;
    }

    // UserData序列化实现
    std::string UserData::serialize() const
    {
        std::ostringstream oss;
        oss << username << "," << password << "," << static_cast<int>(userType) << "," << balance;
        return oss.str();
    }

    UserData UserData::deserialize(const std::string &str)
    {
        UserData user;
        std::istringstream iss(str);
        std::string token;

        try
        {
            if (std::getline(iss, token, ','))
                user.username = token;
            if (std::getline(iss, token, ','))
                user.password = token;
            if (std::getline(iss, token, ','))
                user.userType = static_cast<UserType>(std::stoi(token));
            if (std::getline(iss, token, ','))
                user.balance = std::stod(token);
        }
        catch (const std::exception &e)
        {
            std::cerr << "用户数据反序列化错误: " << e.what() << std::endl;
        }

        return user;
    } // ProductData序列化实现
    std::string ProductData::serialize() const
    {
        std::ostringstream oss;
        oss << id << "," << name << "," << description << "," << type << ","
            << price << "," << originalPrice << "," << quantity << "," << discountRate << "," << sellerUsername;

        // 序列化额外属性
        oss << "," << attributes.size();
        for (const auto &attr : attributes)
        {
            oss << "," << attr.first << "=" << attr.second;
        }

        return oss.str();
    }

    ProductData ProductData::deserialize(const std::string &str)
    {
        ProductData product;
        std::istringstream iss(str);
        std::string token;
        try
        {
            if (std::getline(iss, token, ','))
                product.id = token;
            if (std::getline(iss, token, ','))
                product.name = token;
            if (std::getline(iss, token, ','))
                product.description = token;
            if (std::getline(iss, token, ','))
                product.type = token;
            if (std::getline(iss, token, ','))
                product.price = std::stod(token);
            if (std::getline(iss, token, ','))
                product.originalPrice = std::stod(token);
            if (std::getline(iss, token, ','))
                product.quantity = std::stoi(token);
            if (std::getline(iss, token, ','))
                product.discountRate = std::stod(token);
            if (std::getline(iss, token, ','))
                product.sellerUsername = token;

            // 解析额外属性
            int attrCount = 0;
            if (std::getline(iss, token, ','))
            {
                attrCount = std::stoi(token);
            }

            for (int i = 0; i < attrCount; ++i)
            {
                if (std::getline(iss, token, ','))
                {
                    size_t pos = token.find('=');
                    if (pos != std::string::npos)
                    {
                        std::string key = token.substr(0, pos);
                        std::string value = token.substr(pos + 1);
                        product.attributes[key] = value;
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "商品数据反序列化错误: " << e.what() << std::endl;
        }

        return product;
    }

    // CartItemData序列化实现
    std::string CartItemData::serialize() const
    {
        std::ostringstream oss;
        oss << productId << ";" << productName << ";" << quantity << ";"
            << priceAtAddition << ";" << sellerUsername; // Changed delimiter to ;
        return oss.str();
    }

    CartItemData CartItemData::deserialize(const std::string &str)
    {
        CartItemData item;
        std::istringstream iss(str);
        std::string token;

        try
        {
            if (std::getline(iss, token, ';')) // Changed delimiter to ;
                item.productId = token;
            if (std::getline(iss, token, ';')) // Changed delimiter to ;
                item.productName = token;
            if (std::getline(iss, token, ';')) // Changed delimiter to ;
                item.quantity = std::stoi(token);
            if (std::getline(iss, token, ';')) // Changed delimiter to ;
                item.priceAtAddition = std::stod(token);
            if (std::getline(iss, token, ';')) // Changed delimiter to ;
                item.sellerUsername = token;
        }
        catch (const std::exception &e)
        {
            std::cerr << "购物车项数据反序列化错误: " << e.what() << std::endl;
        }

        return item;
    }

    // OrderData序列化实现
    std::string OrderData::serialize() const
    {
        std::ostringstream oss;
        oss << orderId << "," << customerUsername << "," << totalAmount << ","
            << static_cast<int>(status) << "," << timestamp << "," << items.size();

        for (const auto &item : items)
        {
            oss << "|" << item.serialize(); // 使用不同的分隔符避免冲突
        }

        return oss.str();
    }

    OrderData OrderData::deserialize(const std::string &str)
    {
        OrderData order;
        std::istringstream iss(str);
        std::string token;

        try
        {
            if (std::getline(iss, token, ','))
                order.orderId = token;
            if (std::getline(iss, token, ','))
                order.customerUsername = token;
            if (std::getline(iss, token, ','))
                order.totalAmount = std::stod(token);
            if (std::getline(iss, token, ','))
                order.status = static_cast<OrderStatus>(std::stoi(token));
            if (std::getline(iss, token, ','))
                order.timestamp = std::stoll(token);

            int itemCount = 0;
            if (std::getline(iss, token, '|'))
            {
                itemCount = std::stoi(token);
            }
            if (itemCount > 0)
            {
                std::string allItemsStringSegment;
                std::getline(iss, allItemsStringSegment);

                if (!allItemsStringSegment.empty())
                {
                    std::istringstream itemStream(allItemsStringSegment);
                    std::string singleItemSerialized;

                    for (int i = 0; i < itemCount; ++i)
                    {
                        if (std::getline(itemStream, singleItemSerialized, '|'))
                        {
                            if (!singleItemSerialized.empty())
                            {
                                order.items.push_back(CartItemData::deserialize(singleItemSerialized));
                            }
                            else
                            {
                                std::cerr << "Warning: Order " << order.orderId << ": Encountered an empty string segment for item at index " << i
                                          << " (expected " << itemCount << " total items)." << std::endl;
                            }
                        }
                        else
                        {
                            std::cerr << "Error: Order " << order.orderId << ": Expected " << itemCount
                                      << " items, but could only read " << i
                                      << " segments. Data may be malformed or truncated." << std::endl;
                            break;
                        }
                    }
                }
                else
                {
                    std::cerr << "Error: Order " << order.orderId << ": Expected item data starting with '|' for "
                              << itemCount << " items, but found: \"" << (allItemsStringSegment.length() > 20 ? allItemsStringSegment.substr(0, 20) + "..." : allItemsStringSegment) << "\"" << std::endl;
                }
            }
            // 如果 itemCount 为 0，不需要读取任何订单项数据，这是正常情况
        }
        catch (const std::exception &e)
        {
            std::cerr << "订单数据反序列化错误: " << e.what() << std::endl;
        }

        return order;
    }

} // namespace Protocol
