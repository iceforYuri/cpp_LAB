#include "client_app.h"
#include "client_connection.h"
#include "client_ui.h"

/**
 * 主函数，创建并运行客户端应用
 */
int main()
{
    SetConsoleOutputCP(65001); // 设置控制台输出编码为 UTF-8

    // 创建并运行客户端应用
    ClientApp app;
    return app.run();
}
