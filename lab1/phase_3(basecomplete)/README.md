# 网络版电子商城系统

本系统是一个基于socket通信的客户端-服务器架构的电子商城系统。客户端通过网络与服务器通信，服务器负责处理和存储所有数据，实现用户管理、商品管理和订单处理等功能。

## 系统架构

* **服务器端**：负责数据存储、业务逻辑处理，接收和响应客户端请求
* **客户端**：提供用户界面，通过网络发送请求给服务器并显示结果

## 功能特点

1. **用户管理**：
   - 用户注册、登录、退出
   - 支持消费者和商家两种用户类型

2. **商品管理**：
   - 浏览商品列表
   - 查看商品详情
   - 商家可管理自己的商品（添加、修改、删除等）

3. **购物功能**：
   - 消费者可将商品添加到购物车
   - 查看购物车内容
   - 结算购物车，创建订单

4. **订单处理**：
   - 服务器异步处理订单
   - 更新库存和用户余额
   - 记录订单状态

## 如何编译和运行

### 编译

在VS Code中，你可以通过任务（Tasks）来编译服务器和客户端：

1. 编译服务器：按下`Ctrl+Shift+B`，选择`C/C++: g++.exe 编译服务器`
2. 编译客户端：按下`Ctrl+Shift+B`，选择`C/C++: g++.exe 编译客户端`

或者在终端中手动编译：

```powershell
# 编译服务器
g++ -fdiagnostics-color=always -g server.cpp user/user.cpp store/store.cpp order/order.cpp ordermanager/ordermanager.cpp -o server.exe -lws2_32

# 编译客户端
g++ -fdiagnostics-color=always -g client.cpp -o client.exe -lws2_32
```

### 运行

1. 首先启动服务器：

```powershell
.\server.exe
```

2. 然后在另一个终端窗口启动客户端：

```powershell
.\client.exe
```

可以启动多个客户端实例同时连接到服务器。

## 通信协议

客户端和服务器之间使用简单的基于文本的协议通信，格式为`COMMAND|param1|param2|...`

### 主要命令：

1. **登录**：`LOGIN|username|password`
2. **注册**：`REGISTER|username|password|type`
3. **获取商品列表**：`GET_PRODUCTS`
4. **获取商品详情**：`GET_PRODUCT_DETAIL|productName|sellerUsername`
5. **获取购物车**：`GET_CART|username`
6. **添加到购物车**：`ADD_TO_CART|username|productName|sellerUsername|quantity`
7. **结算购物车**：`CHECKOUT|username`

## 系统架构优势

1. **安全性**：所有数据操作都在服务器端进行，客户端无法直接访问数据文件
2. **扩展性**：可以支持多个客户端同时连接和操作
3. **性能**：通过多线程处理客户端请求和订单处理，提高系统响应速度
4. **简化客户端**：客户端只负责UI交互，不需要处理复杂的业务逻辑
5. **集中管理**：所有数据都在服务器端集中存储和管理，便于备份和维护

## 注意事项

- 启动客户端前必须先启动服务器
- 默认服务器IP为127.0.0.1（本地），端口为8888
- 如需连接到其他服务器，需修改客户端代码中的IP地址
