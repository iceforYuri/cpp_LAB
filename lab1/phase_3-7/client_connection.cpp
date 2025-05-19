#include "client_connection.h"
#include "client_app.h"

ClientConnection::ClientConnection(ClientApp *app) : m_clientSocket(INVALID_SOCKET), m_isConnected(false), m_app(app)
{
}

ClientConnection::~ClientConnection()
{
    disconnect();
}

bool ClientConnection::connectToServer()
{
    // 如果已经连接，则返回true
    if (m_isConnected)
    {
        return true;
    }

    // 初始化Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup 失败: " << result << std::endl;
        return false;
    }

    // 创建socket
    m_clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_clientSocket == INVALID_SOCKET)
    {
        std::cerr << "创建socket失败: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    // 连接到服务器
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 本地回环地址
    serverAddr.sin_port = htons(8888);                   // 服务器端口

    if (connect(m_clientSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "连接到服务器失败: " << WSAGetLastError() << std::endl;
        closesocket(m_clientSocket);
        WSACleanup();
        return false;
    }

    m_isConnected = true;
    std::cout << "成功连接到服务器。" << std::endl;
    return true;
}

bool ClientConnection::sendRequest(const std::string &request, std::string &response)
{
    if (!m_isConnected)
    {
        std::cerr << "未连接到服务器。" << std::endl;
        return false;
    }

    // 发送请求
    int bytesSent = send(m_clientSocket, request.c_str(), request.size(), 0);
    if (bytesSent == SOCKET_ERROR)
    {
        std::cerr << "发送请求失败: " << WSAGetLastError() << std::endl;
        return false;
    }

    // 接收响应
    char buffer[4096] = {0};
    int bytesReceived = recv(m_clientSocket, buffer, sizeof(buffer), 0);

    if (bytesReceived == SOCKET_ERROR)
    {
        std::cerr << "接收响应失败: " << WSAGetLastError() << std::endl;
        return false;
    }

    response = std::string(buffer, bytesReceived);
    return true;
}

void ClientConnection::disconnect()
{
    if (m_isConnected)
    {
        closesocket(m_clientSocket);
        WSACleanup();
        m_isConnected = false;
        std::cout << "已断开与服务器的连接。" << std::endl;
    }
}

bool ClientConnection::isConnected() const
{
    return m_isConnected;
}
