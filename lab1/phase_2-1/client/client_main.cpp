#include <iostream>
#include <string>
#include "client_ui.h"

int main(int argc, char *argv[])
{
    std::string serverHost = "127.0.0.1";
    int serverPort = 8888;

    // 处理命令行参数
    if (argc > 1)
    {
        serverHost = argv[1];
    }

    if (argc > 2)
    {
        try
        {
            serverPort = std::stoi(argv[2]);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Invalid port number. Using default port 8888." << std::endl;
        }
    }

    std::cout << "Starting client, connecting to " << serverHost << ":" << serverPort << std::endl;

    // 创建客户端UI
    ClientUI clientUI(serverHost, serverPort);

    // 启动客户端
    clientUI.run();

    return 0;
}
