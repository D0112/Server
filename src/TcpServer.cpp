#include "TcpServer.h"
#include "TcpConnection.h"
#include "Log.h"
TcpServer::TcpServer(int threadNum, unsigned short port)
{
    m_port = port;
    m_threadNum = threadNum;
    setListen(port);
    m_mainLoop = new EventLoop;
    m_pool = new ThreadPool(m_mainLoop, threadNum);
}

int TcpServer::acceptConn()
{
    int cfd = accept(m_lfd, NULL, NULL);
    if (cfd == -1)
    {
        perror("accept");
        return -1;
    }
    EventLoop *evLoop = m_pool->takeWorkerEventLoop();
    new TcpConnection(evLoop, cfd);
    return 0;
}

void TcpServer::setListen(unsigned short port)
{
    m_lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_lfd == -1)
    {
        perror("socket");
        return;
    }
    int opt = 1;
    int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret == -1)
    {
        perror("setsockopt");
        return;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(m_lfd, (sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("bind");
        return;
    }
    ret = listen(m_lfd, 128);
    if (ret == -1)
    {
        perror("listen");
        return;
    }
}

void TcpServer::run()
{
    Debug("服务器程序已经启动了...");
    m_pool->run();
    auto obj = bind(&TcpServer::acceptConn, this);
    Channel *channel = new Channel(m_lfd, FDtype::ReadEvent, obj, nullptr, nullptr, this);
    m_mainLoop->addTask(channel, TaskType::ADD);
    m_mainLoop->run();
}
