# 客户端-服务器通信协议

本文档详细说明了电子商城系统中客户端和服务器之间的通信协议。所有通信都使用基于文本的协议，采用`|`字符作为字段分隔符。

## 协议格式

### 请求格式
```
COMMAND|param1|param2|...
```

### 响应格式
```
STATUS|message|data1|data2|...
```

## 用户管理相关命令

### 1. 登录请求
**请求**:
```
LOGIN|username|password
```

**成功响应**:
```
LOGIN_SUCCESS|username|userType
```

**失败响应**:
```
LOGIN_FAILED|错误信息
```

### 2. 注册请求
**请求**:
```
REGISTER|username|password|userType
```
- userType: `customer` 或 `seller`

**成功响应**:
```
REGISTER_SUCCESS|username
```

**失败响应**:
```
REGISTER_FAILED|错误信息
```

## 商品相关命令

### 3. 获取所有商品列表
**请求**:
```
GET_PRODUCTS
```

**成功响应**:
```
PRODUCTS|商品数量|商品1名称,类型,价格,库存,商家|商品2名称,类型,价格,库存,商家|...
```

### 4. 获取商品详情
**请求**:
```
GET_PRODUCT_DETAIL|productName|sellerUsername
```

**成功响应** (基本信息):
```
PRODUCT_DETAIL|商品名称|类型|描述|价格|库存|商家
```

**成功响应** (书籍):
```
PRODUCT_DETAIL|商品名称|Book|描述|价格|库存|商家|作者|ISBN
```

**成功响应** (服装):
```
PRODUCT_DETAIL|商品名称|Clothing|描述|价格|库存|商家|尺寸|颜色
```

**成功响应** (食品):
```
PRODUCT_DETAIL|商品名称|Food|描述|价格|库存|商家|保质期
```

**失败响应**:
```
PRODUCT_NOT_FOUND|错误信息
```

## 购物车相关命令

### 5. 获取购物车
**请求**:
```
GET_CART|username
```

**成功响应**:
```
CART|商品数量|商品1名称,数量,价格,商家|商品2名称,数量,价格,商家|...
```

**失败响应**:
```
CART_ERROR|错误信息
```

### 6. 添加商品到购物车
**请求**:
```
ADD_TO_CART|username|productName|sellerUsername|quantity
```

**成功响应**:
```
CART_UPDATED|添加成功
```

**失败响应**:
```
CART_ERROR|错误信息
```

### 7. 结算购物车
**请求**:
```
CHECKOUT|username
```

**成功响应**:
```
CHECKOUT_SUCCESS|订单已提交|orderId
```

**失败响应**:
```
CHECKOUT_ERROR|错误信息
```

## 商家商品管理相关命令

### 8. 添加商品 (书籍)
**请求**:
```
ADD_BOOK|username|name|description|price|quantity|author|isbn
```

**成功响应**:
```
PRODUCT_ADDED|添加成功
```

**失败响应**:
```
PRODUCT_ERROR|错误信息
```

### 9. 添加商品 (服装)
**请求**:
```
ADD_CLOTHING|username|name|description|price|quantity|size|color
```

**成功响应**:
```
PRODUCT_ADDED|添加成功
```

**失败响应**:
```
PRODUCT_ERROR|错误信息
```

### 10. 添加商品 (食品)
**请求**:
```
ADD_FOOD|username|name|description|price|quantity|expirationDate
```

**成功响应**:
```
PRODUCT_ADDED|添加成功
```

**失败响应**:
```
PRODUCT_ERROR|错误信息
```

### 11. 修改商品价格
**请求**:
```
UPDATE_PRODUCT_PRICE|username|productName|newPrice
```

**成功响应**:
```
PRODUCT_UPDATED|修改成功
```

**失败响应**:
```
PRODUCT_ERROR|错误信息
```

### 12. 修改商品库存
**请求**:
```
UPDATE_PRODUCT_QUANTITY|username|productName|newQuantity
```

**成功响应**:
```
PRODUCT_UPDATED|修改成功
```

**失败响应**:
```
PRODUCT_ERROR|错误信息
```

### 13. 设置商品折扣
**请求**:
```
SET_PRODUCT_DISCOUNT|username|productName|discountRate
```

**成功响应**:
```
PRODUCT_UPDATED|设置成功
```

**失败响应**:
```
PRODUCT_ERROR|错误信息
```

## 用户余额相关命令

### 14. 查询余额
**请求**:
```
CHECK_BALANCE|username
```

**成功响应**:
```
BALANCE|余额数值
```

**失败响应**:
```
BALANCE_ERROR|错误信息
```

### 15. 充值
**请求**:
```
DEPOSIT|username|amount
```

**成功响应**:
```
BALANCE_UPDATED|充值成功|新余额
```

**失败响应**:
```
BALANCE_ERROR|错误信息
```

## 错误处理

所有错误响应都会以相应命令的错误标识开头，如LOGIN_FAILED, REGISTER_FAILED等，后面跟随具体的错误信息。客户端应当正确处理这些错误响应并向用户显示适当的信息。

## 注意事项

1. 所有价格、数量等数值类型在传输时都转换为字符串
2. 请确保字符串中不包含`|`字符，否则会导致协议解析错误
3. 服务器会检查所有请求的有效性，包括用户权限检查
4. 日期格式统一使用YYYY-MM-DD格式
