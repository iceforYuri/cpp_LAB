#include "socket.h"

// Socket基类实现
Socket::Socket() : socketFd(INVALID_SOCKET), initialized(false) {}

Socket::~Socket()
{
    if (socketFd != INVALID_SOCKET)
    {
        closesocket(socketFd);
    }
}

bool Socket::initializeWinsock()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    return (result == 0);
}

void Socket::cleanupWinsock()
{
    WSACleanup();
}

// ServerSocket实现
ServerSocket::ServerSocket(const std::string &host, int port, int backlog)
    : Socket(), listening(false), backlog(backlog), host(host), port(port)
{

    if (!initializeWinsock())
    {
        lastError = "Failed to initialize Winsock";
        return;
    }

    socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketFd == INVALID_SOCKET)
    {
        lastError = "Failed to create server socket";
        return;
    }

    initialized = true;
}

ServerSocket::~ServerSocket()
{
    close();
    cleanupWinsock();
}

bool ServerSocket::bindAndListen()
{
    if (!initialized)
    {
        return false;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    // 将主机名转换为IP地址
    if (host == "localhost" || host == "127.0.0.1")
    {
        serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    else if (host == "0.0.0.0")
    {
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        inet_pton(AF_INET, host.c_str(), &(serverAddr.sin_addr));
    }

    // 绑定socket到地址和端口
    if (bind(socketFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        lastError = "Failed to bind server socket";
        return false;
    }

    // 开始监听
    if (listen(socketFd, backlog) == SOCKET_ERROR)
    {
        lastError = "Failed to listen on server socket";
        return false;
    }

    listening = true;
    return true;
}

SOCKET ServerSocket::acceptClient(std::string &clientIP)
{
    if (!listening)
    {
        lastError = "Server socket is not listening";
        return INVALID_SOCKET;
    }

    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    SOCKET clientSocket = accept(socketFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (clientSocket == INVALID_SOCKET)
    {
        lastError = "Failed to accept client connection";
        return INVALID_SOCKET;
    }

    // 获取客户端IP地址
    char ipBuffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), ipBuffer, INET_ADDRSTRLEN);
    clientIP = ipBuffer;

    return clientSocket;
}

void ServerSocket::close()
{
    if (socketFd != INVALID_SOCKET)
    {
        closesocket(socketFd);
        socketFd = INVALID_SOCKET;
    }
    listening = false;
}

// ClientSocket实现
ClientSocket::ClientSocket(const std::string &host, int port)
    : Socket(), serverHost(host), serverPort(port), connected(false)
{

    if (!initializeWinsock())
    {
        lastError = "Failed to initialize Winsock";
        return;
    }

    socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketFd == INVALID_SOCKET)
    {
        lastError = "Failed to create client socket";
        return;
    }

    initialized = true;
}

ClientSocket::~ClientSocket()
{
    close();
    cleanupWinsock();
}

bool ClientSocket::connectToServer()
{
    if (!initialized)
    {
        return false;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);

    // 将主机名转换为IP地址
    if (serverHost == "localhost")
    {
        inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr));
    }
    else
    {
        inet_pton(AF_INET, serverHost.c_str(), &(serverAddr.sin_addr));
    }

    // 连接到服务器
    if (connect(socketFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        lastError = "Failed to connect to server";
        return false;
    }

    connected = true;
    return true;
}

bool ClientSocket::sendData(const std::string &data)
{
    if (!connected)
    {
        lastError = "Not connected to server";
        return false;
    }

    const char *buffer = data.c_str();
    int totalSent = 0;
    int remaining = data.length();

    // 确保所有数据都被发送
    while (totalSent < data.length())
    {
        int sentBytes = send(socketFd, buffer + totalSent, remaining, 0);
        if (sentBytes == SOCKET_ERROR)
        {
            lastError = "Send failed";
            return false;
        }

        totalSent += sentBytes;
        remaining -= sentBytes;
    }

    return true;
}

std::string ClientSocket::receiveData(int bufferSize)
{
    if (!connected)
    {
        lastError = "Not connected to server";
        return "";
    }

    std::vector<char> buffer(bufferSize);
    std::string result;

    int bytesReceived = recv(socketFd, buffer.data(), bufferSize - 1, 0);
    if (bytesReceived == SOCKET_ERROR)
    {
        lastError = "Receive failed";
        return "";
    }

    buffer[bytesReceived] = '\0';
    result = buffer.data();

    return result;
}

void ClientSocket::close()
{
    if (socketFd != INVALID_SOCKET)
    {
        closesocket(socketFd);
        socketFd = INVALID_SOCKET;
    }
    connected = false;
}

// ConnectionHandler实现
ConnectionHandler::ConnectionHandler(SOCKET socket, const std::string &ip)
    : clientSocket(socket), clientIP(ip) {}

ConnectionHandler::~ConnectionHandler()
{
    close();
}

void ConnectionHandler::handleRequest(const std::function<std::string(const std::string &, const std::string &)> &requestHandler)
{
    std::string request = receiveRequest();
    if (!request.empty())
    {
        std::string response = requestHandler(request, clientIP);
        sendResponse(response);
    }
}

bool ConnectionHandler::sendResponse(const std::string &response)
{
    const char *buffer = response.c_str();
    int totalSent = 0;
    int remaining = response.length();

    // 确保所有数据都被发送
    while (totalSent < response.length())
    {
        int sentBytes = send(clientSocket, buffer + totalSent, remaining, 0);
        if (sentBytes == SOCKET_ERROR)
        {
            return false;
        }

        totalSent += sentBytes;
        remaining -= sentBytes;
    }

    return true;
}

std::string ConnectionHandler::receiveRequest(int bufferSize)
{
    std::vector<char> buffer(bufferSize);
    std::string result;

    int bytesReceived = recv(clientSocket, buffer.data(), bufferSize - 1, 0);
    if (bytesReceived == SOCKET_ERROR || bytesReceived == 0)
    {
        return "";
    }

    buffer[bytesReceived] = '\0';
    result = buffer.data();

    return result;
}

void ConnectionHandler::close()
{
    if (clientSocket != INVALID_SOCKET)
    {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
}
