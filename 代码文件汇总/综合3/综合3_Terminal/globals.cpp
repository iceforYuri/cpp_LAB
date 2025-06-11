#include <string>
#include "ordermanager/ordermanager.h"

// 全局变量定义
// 这些变量被多个源文件引用，在重构过程中保持兼容性
namespace
{
    // 这些变量仅在此文件中可见，但我们提供全局访问函数
    std::string g_userFile = "./user/users.txt";
    std::string g_storeFile = "./store";
    std::string g_cartFile = "./user/carts";
    std::string g_orderDir = "./order/orders";
}

// 全局变量声明
// 使用 extern 使这些变量在其他源文件中可见
extern const std::string USER_FILE = g_userFile;
extern const std::string STORE_FILE = g_storeFile;
extern const std::string CART_FILE = g_cartFile;
extern const std::string ORDER_DIR = g_orderDir;

// 全局订单管理器对象
OrderManager g_orderManager(ORDER_DIR);
