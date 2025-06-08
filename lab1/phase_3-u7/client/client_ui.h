#ifndef CLIENT_UI_H
#define CLIENT_UI_H

#include "../imgui/imgui.h" // 必须首先包含 ImGui
#include "../network/client.h"
#include "../network/protocol.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <cstdarg> // 添加支持可变参数函数

// 前向声明 - 确保正确的类型声明
typedef struct GLFWwindow GLFWwindow;

// 客户端UI系统主类
class ClientUI
{
private:
  // GLFW窗口
  GLFWwindow *window = nullptr;

  // 窗口尺寸
  int width = 800;
  int height = 600;
  int originalWidth = 800;  // 原始窗口宽度
  int originalHeight = 600; // 原始窗口高度
  float uiScale = 1.0f;     // UI缩放比例

  // UI状态
  bool shouldClose = false;
  int currentPage = 0; // 0=主菜单, 1=用户菜单, 2=商城
  Protocol::UserType userType = Protocol::UserType::CUSTOMER;

  // 商品滚动位置
  float scrollY = 0.0f;

  // 登录输入缓冲区
  char usernameBuffer[128] = {0};
  char passwordBuffer[128] = {0};

  // 用户信息
  Protocol::UserData currentUser;
  bool isLoggedIn = false; // 商品相关
  char searchBuffer[128] = {0};
  char lastSearchBuffer[128] = {0}; // 用于跟踪搜索框内容变化
  std::vector<Protocol::ProductData> allProducts;
  std::vector<Protocol::ProductData> searchResults;

  // 购买商品
  char buyProductName[128] = {0};
  int buyQuantity = 1; // 商品管理 (商家)
  char newProductName[128] = {0};
  char newProductDesc[256] = {0};
  float newProductPrice = 0.0f;
  int newProductQuantity = 1;
  float newProductDiscount = 0.0f;
  int newProductType = 0;          // 商品类型索引 (0=图书, 1=服装, 2=食品, 3=其他)
  char newProductAttr1[128] = {0}; // 第一个属性字段
  char newProductAttr2[128] = {0}; // 第二个属性字段

  // 购物车
  std::vector<Protocol::CartItemData> cartItems;
  bool showCartWindow = false; // 订单
  std::vector<Protocol::OrderData> userOrders;
  bool showOrderWindow = false;
  Protocol::OrderData selectedOrder;

  // 网络客户端
  std::unique_ptr<NetworkClient> networkClient;
  // 状态信息
  std::string statusMessage;
  bool showErrorMessage = false;
  std::string errorMessage;

  // 确认对话框状态
  bool showPurchaseConfirmDialog = false;
  bool showDirectPurchaseConfirmDialog = false;
  bool showCartCheckoutConfirmDialog = false;
  Protocol::ProductData productToPurchase;
  int quantityToPurchase = 1; // 成功消息弹窗状态
  bool showAddToCartSuccessPopup = false;
  std::string addToCartSuccessMessage; // 密码修改弹窗状态
  bool showPasswordChangeDialog = false;
  bool showPasswordChangeSuccessDialog = false;
  bool showPasswordChangeFailDialog = false;

  // 充值弹窗状态
  bool showRechargeDialog = false;
  bool showRechargeSuccessDialog = false;
  bool showRechargeFailDialog = false;
  float rechargeAmount = 0.0f;
  std::string rechargeMessage;

  // 主题管理
  bool isDarkTheme = true;
  ImVec4 primaryColor;
  ImVec4 secondaryColor;
  ImVec4 backgroundColor;

  // 渲染方法
  void renderMainMenu();
  void renderUserMenu();
  void renderStore();
  void renderUserRegister();
  void renderUserLogin();
  void renderStoreCustomer();
  void renderStoreSeller();
  void renderStoreAdmin();
  void renderProductList(const std::vector<Protocol::ProductData> &products, const std::string &title);
  void renderCartWindow();
  void renderOrderWindow();
  void renderOrderDetails(const Protocol::OrderData &order);
  void renderAddProductWindow();
  void renderStatusMessages(); // 对话框和弹窗方法
  void renderPurchaseConfirmDialog();
  void renderDirectPurchaseConfirmDialog();
  void renderCartCheckoutConfirmDialog();
  void renderAddToCartSuccessPopup();
  void renderPasswordChangeDialog();
  void renderPasswordChangeSuccessDialog();
  void renderPasswordChangeFailDialog();
  void renderRechargeDialog();
  void renderRechargeSuccessDialog();
  void renderRechargeFailDialog();
  // UI优化相关方法
  void setupModernTheme();
  void setupDarkTheme();
  void setupLightTheme();
  bool renderModernButton(const char *label, ImVec2 size, bool primary = false);
  void renderCard(const char *title, const char *content, bool collapsible = false);
  void renderSearchBox();
  void renderProductCard(const Protocol::ProductData &product);
  void renderStatusBar();

  // 窗口响应式方法
  void updateWindowSize();
  void updateUIScale();
  ImVec2 scaleSize(const ImVec2 &size);
  float scaleFloat(float value);
  static void framebufferSizeCallback(GLFWwindow *window, int width, int height);

public:
  ClientUI(int width = 800, int height = 600);
  ~ClientUI();

  // 初始化UI系统
  bool init();

  // 连接到服务器
  bool connectToServer(const std::string &address = "127.0.0.1", int port = 8888); // 断开服务器连接
  void disconnectFromServer();

  // 主循环
  void mainLoop();

  // 是否应该关闭
  bool isClosing() const { return shouldClose; }

  // 用户操作
  bool login(const std::string &username, const std::string &password);
  bool registerUser(const std::string &username, const std::string &password, Protocol::UserType userType);
  void logout();

  // 商品操作
  bool addProduct(const Protocol::ProductData &product);
  void refreshProducts();
  void refreshCart();
  void refreshOrders();
  void refreshUserOrders();
  void performSearch();
  void addProductToCart(const Protocol::ProductData &product);
  void updateCartQuantity(const std::string &productId, int newQuantity);
  void removeFromCart(const std::string &productId);
  void checkout();
  void checkoutWithInventoryLock();
  void addToCartByName(const std::string &productName, int quantity);
  void purchaseProduct(const std::string &productName, int quantity);
  void clearMessages();

  // 状态和错误消息方法
  void setStatus(const std::string &message);
  void setError(const std::string &error);
  void setStatus(const char *format, ...);
  void setError(const char *format, ...); // 获取当前用户信息
  const Protocol::UserData &getCurrentUser() const { return currentUser; }
  bool getIsLoggedIn() const { return isLoggedIn; }
};

#endif // CLIENT_UI_H
