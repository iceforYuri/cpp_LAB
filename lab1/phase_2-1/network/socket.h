#ifndef SOCKET_H
#define SOCKET_H

#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class Socket
{
protected:
    SOCKET socketFd;
    bool initialized;
    std::string lastError;

public:
    Socket();
    virtual ~Socket();

    bool isInitialized() const { return initialized; }
    std::string getLastError() const { return lastError; }

    // 初始化Winsock
    static bool initializeWinsock();
    // 清理Winsock
    static void cleanupWinsock();
};

class ServerSocket : public Socket
{
private:
    bool listening;
    int backlog;
    std::string host;
    int port;

public:
    ServerSocket(const std::string &host = "127.0.0.1", int port = 8888, int backlog = 10);
    ~ServerSocket();

    // 绑定并监听端口
    bool bindAndListen();

    // 接受客户端连接
    SOCKET acceptClient(std::string &clientIP);

    // 关闭服务器socket
    void close();

    bool isListening() const { return listening; }
};

class ClientSocket : public Socket
{
private:
    std::string serverHost;
    int serverPort;
    bool connected;

public:
    ClientSocket(const std::string &host = "127.0.0.1", int port = 8888);
    ~ClientSocket();

    // 连接到服务器
    bool connectToServer();

    // 发送数据
    bool sendData(const std::string &data);

    // 接收数据
    std::string receiveData(int bufferSize = 4096);

    // 关闭客户端socket
    void close();

    bool isConnected() const { return connected; }
};

class ConnectionHandler
{
private:
    SOCKET clientSocket;
    std::string clientIP;

public:
    ConnectionHandler(SOCKET socket, const std::string &ip);
    ~ConnectionHandler();

    // 处理客户端请求
    void handleRequest(const std::function<std::string(const std::string &, const std::string &)> &requestHandler);

    // 发送数据到客户端
    bool sendResponse(const std::string &response);

    // 从客户端接收数据
    std::string receiveRequest(int bufferSize = 4096);

    // 关闭连接
    void close();

    std::string getClientIP() const { return clientIP; }
};

#endif // SOCKET_H
