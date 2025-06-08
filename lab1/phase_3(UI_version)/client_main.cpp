#include <iostream>
#include <string>
#include "client/client_ui.h"
#include <windows.h>

int main()
{
    // 设置控制台输出编码为UTF-8
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    std::cout << "=== 电子商城客户端 ===" << std::endl;

    // 创建客户端UI
    ClientUI clientUI(800, 600);

    // 初始化UI
    if (!clientUI.init())
    {
        std::cerr << "客户端UI初始化失败，程序退出" << std::endl;
        return 1;
    }

    // 连接到服务器
    std::string serverAddress = "127.0.0.1";
    int serverPort = 8888;

    std::cout << "正在连接到服务器 " << serverAddress << ":" << serverPort << "..." << std::endl;

    if (!clientUI.connectToServer(serverAddress, serverPort))
    {
        std::cerr << "无法连接到服务器，请确保服务器正在运行" << std::endl;
        std::cout << "按任意键退出..." << std::endl;
        std::cin.get();
        return 1;
    }

    std::cout << "已成功连接到服务器" << std::endl;

    // 启动客户端主循环
    clientUI.mainLoop();

    // 断开连接
    clientUI.disconnectFromServer();

    std::cout << "客户端已退出" << std::endl;
    return 0;
}
