#ifndef REQUEST_PROCESSOR_H
#define REQUEST_PROCESSOR_H

#include <string>
#include <vector>
#include <sstream>

#include "user/user.h"
#include "store/store.h"
#include "order/order.h"
#include "ordermanager/ordermanager.h"

// 前向声明
class ServerApp;
class ClientHandler;

/**
 * @brief 处理客户端各种请求的处理器
 */
class RequestProcessor
{
private:
  ServerApp *m_server;
  ClientHandler *m_clientHandler;

public:
  /**
   * @brief 构造函数
   * @param server 服务器应用指针
   * @param clientHandler 客户端处理器指针
   */
  RequestProcessor(ServerApp *server, ClientHandler *clientHandler);
  /**
   * @brief 处理登录请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processLoginRequest(const std::string &request);

  /**
   * @brief 处理注册请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processRegisterRequest(const std::string &request);

  /**
   * @brief 处理获取商品列表请求
   * @return 响应字符串
   */
  std::string processGetProductsRequest() const;

  /**
   * @brief 处理获取商品详情请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processGetProductDetailRequest(const std::string &request) const;
  /**
   * @brief 处理获取购物车请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processGetCartRequest(const std::string &request) const;

  /**
   * @brief 处理添加到购物车请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processAddToCartRequest(const std::string &request);
  /**
   * @brief 处理从购物车移除请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processRemoveFromCartRequest(const std::string &request);

  /**
   * @brief 处理更新购物车项目请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processUpdateCartItemRequest(const std::string &request);

  /**
   * @brief 处理结算请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processCheckoutRequest(const std::string &request);

  /**
   * @brief 处理登出请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processLogoutRequest(const std::string &request);

  /**
   * @brief 处理获取用户信息请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processGetUserInfoRequest(const std::string &request) const;
  /**
   * @brief 处理检查余额请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processCheckBalanceRequest(const std::string &request) const;

  /**
   * @brief 处理充值请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processDepositRequest(const std::string &request);

  /**
   * @brief 处理搜索商品请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processSearchProductsRequest(const std::string &request) const;

  /**
   * @brief 处理获取商家信息请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processGetSellerInfoRequest(const std::string &request) const;

  /**
   * @brief 处理更改密码请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processChangePasswordRequest(const std::string &request);

  /**
   * @brief 处理添加商品请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processAddProductRequest(const std::string &request);

  /**
   * @brief 处理更新商品请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processUpdateProductRequest(const std::string &request);

  /**
   * @brief 处理删除商品请求
   * @param request 请求字符串
   * @return 响应字符串
   */
  std::string processRemoveProductRequest(const std::string &request);

  /**
   * @brief 获取服务器应用指针
   * @return 服务器应用指针
   */
  ServerApp *getServer() const;

  /**
   * @brief 获取客户端处理器指针
   * @return 客户端处理器指针
   */
  ClientHandler *getClientHandler() const;
};

#endif // REQUEST_PROCESSOR_H
