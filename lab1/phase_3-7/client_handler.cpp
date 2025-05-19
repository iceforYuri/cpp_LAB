#include "client_handler.h"
#include "server_app.h"
#include "request_processor.h"

ClientHandler::ClientHandler(SOCKET clientSocket, ServerApp *server) : m_clientSocket(clientSocket),
                                                                       m_server(server),
                                                                       m_currentConnectedUser(""),
                                                                       m_clientConnected(true)
{
    m_processor = new RequestProcessor(server, this);
}

ClientHandler::~ClientHandler()
{
    disconnectClient();
    delete m_processor;
}

void ClientHandler::handleClient()
{
    char buffer[4096] = {0};
    std::string request, response;

    std::cout << "客户端已连接，处理中..." << std::endl;

    while (m_clientConnected && m_server->isRunning())
    {
        // 清空缓冲区
        memset(buffer, 0, sizeof(buffer));

        // 接收客户端请求
        int bytesReceived = recv(m_clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0)
        {
            std::cout << "客户端断开连接或接收错误" << std::endl;
            break;
        }

        // 解析请求
        request = std::string(buffer);
        std::cout << "收到请求: " << request << std::endl;

        // 处理请求
        response = processRequest(request);

        // 发送响应
        if (!sendResponse(response))
        {
            break;
        }
    }

    // 断开连接前清理资源
    if (!m_currentConnectedUser.empty())
    {
        m_server->removeLoggedInUser(m_currentConnectedUser);
    }

    disconnectClient();
}

std::string ClientHandler::processRequest(const std::string &request)
{
    std::string command = request.substr(0, request.find("|"));

    if (command == "LOGIN")
    {
        return m_processor->processLoginRequest(request);
    }
    else if (command == "REGISTER")
    {
        return m_processor->processRegisterRequest(request);
    }
    else if (command == "GET_PRODUCTS")
    {
        return m_processor->processGetProductsRequest();
    }
    else if (command == "GET_PRODUCT_DETAIL")
    {
        return m_processor->processGetProductDetailRequest(request);
    }
    else if (command == "GET_CART")
    {
        return m_processor->processGetCartRequest(request);
    }
    else if (command == "ADD_TO_CART")
    {
        return m_processor->processAddToCartRequest(request);
    }
    else if (command == "REMOVE_FROM_CART")
    {
        return m_processor->processRemoveFromCartRequest(request);
    }
    else if (command == "UPDATE_CART_ITEM")
    {
        return m_processor->processUpdateCartItemRequest(request);
    }
    else if (command == "CHECKOUT")
    {
        return m_processor->processCheckoutRequest(request);
    }
    else if (command == "LOGOUT")
    {
        return m_processor->processLogoutRequest(request);
    }
    else if (command == "GET_USER_INFO")
    {
        return m_processor->processGetUserInfoRequest(request);
    }
    else if (command == "CHECK_BALANCE")
    {
        return m_processor->processCheckBalanceRequest(request);
    }
    else if (command == "DEPOSIT")
    {
        return m_processor->processDepositRequest(request);
    }
    else if (command == "SEARCH_PRODUCTS")
    {
        return m_processor->processSearchProductsRequest(request);
    }
    else if (command == "GET_SELLER_INFO")
    {
        return m_processor->processGetSellerInfoRequest(request);
    }
    else if (command == "CHANGE_PASSWORD")
    {
        return m_processor->processChangePasswordRequest(request);
    }
    else if (command == "ADD_PRODUCT")
    {
        return m_processor->processAddProductRequest(request);
    }
    else if (command == "UPDATE_PRODUCT")
    {
        return m_processor->processUpdateProductRequest(request);
    }
    else if (command == "REMOVE_PRODUCT")
    {
        return m_processor->processRemoveProductRequest(request);
    }
    else
    {
        return "ERROR|未知命令";
    }
}

bool ClientHandler::sendResponse(const std::string &response)
{
    int bytesSent = send(m_clientSocket, response.c_str(), response.size(), 0);
    if (bytesSent == SOCKET_ERROR)
    {
        std::cerr << "发送响应失败: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}

void ClientHandler::disconnectClient()
{
    if (m_clientSocket != INVALID_SOCKET)
    {
        closesocket(m_clientSocket);
        m_clientSocket = INVALID_SOCKET;
    }
    m_clientConnected = false;
}

std::string ClientHandler::getCurrentConnectedUser() const
{
    return m_currentConnectedUser;
}

void ClientHandler::setCurrentConnectedUser(const std::string &username)
{
    m_currentConnectedUser = username;
}

bool ClientHandler::isClientConnected() const
{
    return m_clientConnected;
}

ServerApp *ClientHandler::getServer() const
{
    return m_server;
}

SOCKET ClientHandler::getClientSocket() const
{
    return m_clientSocket;
}
