#include "server_app.h"
#include "client_handler.h"
#include "request_processor.h"

// 定义全局变量，解决链接错误
extern const std::string USER_FILE = "./user/users.txt";
extern const std::string CART_FILE = "./user/carts";
extern const std::string ORDER_DIR = "./order/orders";
OrderManager g_orderManager(ORDER_DIR); // 创建订单管理器对象

// ServerApp 实现
ServerApp::ServerApp(
    const std::string &userFile,
    const std::string &storeFile,
    const std::string &cartFile,
    const std::string &orderDir) : m_serverSocket(INVALID_SOCKET),
                                   m_initialized(false),
                                   m_userFile(userFile),
                                   m_storeFile(storeFile),
                                   m_cartFile(cartFile),
                                   m_orderDir(orderDir),
                                   m_store(nullptr),
                                   m_orderManager(nullptr),
                                   m_serverRunning(false)
{
}

ServerApp::~ServerApp()
{
    cleanup();
}

bool ServerApp::initialize()
{
    if (m_initialized)
    {
        return true;
    }

    // 初始化 Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup 失败: " << result << std::endl;
        return false;
    }

    // 创建服务器socket
    m_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_serverSocket == INVALID_SOCKET)
    {
        std::cerr << "创建socket失败: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    // 绑定到本地地址和端口
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888); // 使用8888端口

    if (bind(m_serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "绑定失败: " << WSAGetLastError() << std::endl;
        closesocket(m_serverSocket);
        WSACleanup();
        return false;
    }

    // 开始监听
    if (listen(m_serverSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "监听失败: " << WSAGetLastError() << std::endl;
        closesocket(m_serverSocket);
        WSACleanup();
        return false;
    }

    // 加载用户数据
    m_users = User::loadUsersFromFile(m_userFile);
    std::cout << "已加载 " << m_users.size() << " 个用户数据。" << std::endl;

    // 创建 Store 对象
    m_store = new Store(m_storeFile);
    m_store->loadAllProducts();
    std::cout << "已加载商品数据。" << std::endl;

    // 创建订单管理器
    m_orderManager = new OrderManager(m_orderDir);

    m_initialized = true;
    m_serverRunning = true;

    return true;
}

int ServerApp::run()
{
    if (!m_initialized && !initialize())
    {
        std::cerr << "初始化服务器失败" << std::endl;
        return 1;
    }

    std::cout << "服务器启动，等待连接..." << std::endl;

    // 启动订单处理线程
    m_orderManager->startProcessingThread(*m_store, m_users);
    std::cout << "订单处理线程已启动。" << std::endl;

    // 接受客户端连接
    acceptConnections();

    // 等待所有客户端线程结束
    for (auto &thread : m_clientThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    // 清理资源
    cleanup();

    return 0;
}

void ServerApp::stop()
{
    m_serverRunning = false;

    // 关闭服务器socket以中断acceptConnections循环
    if (m_serverSocket != INVALID_SOCKET)
    {
        closesocket(m_serverSocket);
        m_serverSocket = INVALID_SOCKET;
    }
}

void ServerApp::acceptConnections()
{
    while (m_serverRunning)
    {
        // 接受客户端连接
        SOCKET clientSocket = accept(m_serverSocket, NULL, NULL);

        if (clientSocket == INVALID_SOCKET)
        {
            if (m_serverRunning)
            {
                std::cerr << "接受连接失败: " << WSAGetLastError() << std::endl;
            }
            break;
        }

        // 为每个客户端创建一个新线程
        m_clientThreads.push_back(std::thread([this, clientSocket]()
                                              {
            ClientHandler handler(clientSocket, this);
            handler.handleClient(); }));

        // 分离线程让它独立运行
        m_clientThreads.back().detach();
    }
}

void ServerApp::cleanup()
{
    // 停止服务器运行
    m_serverRunning = false;

    // 关闭服务器socket
    if (m_serverSocket != INVALID_SOCKET)
    {
        closesocket(m_serverSocket);
        m_serverSocket = INVALID_SOCKET;
    }

    // 等待所有客户端线程结束
    for (auto &thread : m_clientThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    // 清理用户数据
    for (auto user : m_users)
    {
        delete user;
    }
    m_users.clear();

    // 清理Store对象
    if (m_store)
    {
        delete m_store;
        m_store = nullptr;
    }

    // 清理OrderManager对象
    if (m_orderManager)
    {
        delete m_orderManager;
        m_orderManager = nullptr;
    }

    // 清理Winsock
    WSACleanup();

    m_initialized = false;
}

std::mutex &ServerApp::getUsersMutex()
{
    return m_usersMutex;
}

std::mutex &ServerApp::getStoreMutex()
{
    return m_storeMutex;
}

std::mutex &ServerApp::getLoggedInUsersMutex()
{
    return m_loggedInUsersMutex;
}

std::vector<User *> &ServerApp::getUsers()
{
    return m_users;
}

Store *ServerApp::getStore() const
{
    return m_store;
}

OrderManager *ServerApp::getOrderManager() const
{
    return m_orderManager;
}

std::set<std::string> &ServerApp::getLoggedInUsers()
{
    return m_loggedInUsers;
}

bool ServerApp::isRunning() const
{
    return m_serverRunning;
}

void ServerApp::addLoggedInUser(const std::string &username)
{
    std::lock_guard<std::mutex> lock(m_loggedInUsersMutex);
    m_loggedInUsers.insert(username);
}

void ServerApp::removeLoggedInUser(const std::string &username)
{
    std::lock_guard<std::mutex> lock(m_loggedInUsersMutex);
    m_loggedInUsers.erase(username);
}

bool ServerApp::isUserLoggedIn(const std::string &username) const
{
    std::lock_guard<std::mutex> lock(const_cast<std::mutex &>(m_loggedInUsersMutex));
    return m_loggedInUsers.find(username) != m_loggedInUsers.end();
}
