@echo off
echo 正在编译电商系统网络版...

rem 创建编译目录
if not exist build mkdir build

rem 编译服务器
echo 正在编译服务器...
g++ -std=c++11 -Wall -I. -o build\server.exe ^
    server\server_main.cpp ^
    server\request_handler.cpp ^
    network\socket.cpp ^
    network\protocol.cpp ^
    user\user.cpp ^
    store\store.cpp ^
    order\order.cpp ^
    ordermanager\ordermanager.cpp ^
    -lws2_32

if %ERRORLEVEL% neq 0 (
    echo 服务器编译失败！
    exit /b %ERRORLEVEL%
)

rem 编译客户端
echo 正在编译客户端...
g++ -std=c++11 -Wall -I. -o build\client.exe ^
    client\client_main.cpp ^
    client\client_api.cpp ^
    client\client_ui.cpp ^
    network\socket.cpp ^
    network\protocol.cpp ^
    -lws2_32

if %ERRORLEVEL% neq 0 (
    echo 客户端编译失败！
    exit /b %ERRORLEVEL%
)

echo 编译完成！
echo 启动服务器: build\server.exe
echo 启动客户端: build\client.exe
