# 电子商城系统重构指南

## 项目概述

本项目是一个电子商城系统的重构版本，将原有的面向过程代码转换为面向对象的结构。主要改进包括：

1. 将非类成员函数转换为类成员函数
2. 为不修改对象状态的方法添加 `const` 修饰符
3. 按功能将类定义和实现分离到不同的文件中
4. 保持原有功能的同时改进代码结构

## 文件结构

### 客户端

- `client_app.h` - 客户端主应用类定义
- `client_connection.h` - 网络连接处理类定义
- `client_ui.h` - 用户界面功能类定义
- `client_app_impl.h` - ClientApp类方法实现
- `client_new.cpp` - 新版客户端主程序

### 服务器端

- `server_app.h` - 服务器主应用类定义
- `server_app.cpp` - ServerApp类方法实现
- `client_handler.h` - 客户端处理类定义
- `client_handler.cpp` - ClientHandler类方法实现
- `request_processor.h` - 请求处理类定义
- `request_processor.cpp` - RequestProcessor类方法实现
- `server_new.cpp` - 新版服务器主程序

### 原始文件 (未修改)

- `client.cpp` - 原始客户端程序
- `server.cpp` - 原始服务器程序
- 其他支持模块 (user, store, order, ordermanager 等)

## 类结构

### 客户端

1. `ClientApp` - 主应用类，管理整个客户端应用的生命周期和状态
   - 成员：登录状态，用户信息，UI和连接组件
   - 方法：各种业务功能 (登录、注册、浏览商品等)

2. `ClientConnection` - 处理网络连接
   - 成员：套接字，连接状态
   - 方法：连接服务器，发送请求，接收响应

3. `ClientUI` - 处理用户界面
   - 成员：对ClientApp的引用
   - 方法：显示菜单，清理输入缓冲区

### 服务器端

1. `ServerApp` - 主应用类，管理服务器的生命周期
   - 成员：套接字，用户列表，商店对象，线程和互斥量
   - 方法：初始化，运行，停止服务器

2. `ClientHandler` - 处理单个客户端连接
   - 成员：客户端套接字，当前用户，请求处理器
   - 方法：接收请求，发送响应，断开连接

3. `RequestProcessor` - 处理各种类型的请求
   - 成员：对ServerApp和ClientHandler的引用
   - 方法：各种请求处理方法 (登录、注册、获取商品等)

## 如何编译和运行

### 新版客户端

```bash
g++ client_new.cpp -o client_new.exe -lws2_32
```

或使用VS Code任务：`C/C++: g++.exe 编译新版客户端`

### 新版服务器

```bash
g++ server_app.cpp client_handler.cpp request_processor.cpp server_new.cpp user/user.cpp store/store.cpp order/order.cpp ordermanager/ordermanager.cpp -o server_new.exe -lws2_32
```

或使用VS Code任务：`C/C++: g++.exe 编译新版服务器`

## 注意事项

1. 新版本完全兼容原有功能，可以与原版本互操作
2. 为保持向后兼容，保留了原始文件，可以根据需要使用
3. 建议使用新版本，因为代码结构更清晰，更易于维护和扩展
