#include "server_app.h"
#include "client_handler.h"
#include "request_processor.h"

int main()
{
    SetConsoleOutputCP(65001); // 设置控制台输出编码为 UTF-8
    SetConsoleCP(65001);       // 设置控制台输入编码为 UTF-8

    // 创建并运行服务器应用
    ServerApp server;
    return server.run();
}
