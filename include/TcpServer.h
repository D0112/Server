#pragma once
#include "EventLoop.h"
#include "ThreadPool.h"
#include <sys/socket.h>
#include <arpa/inet.h>

class TcpServer
{
public:
    TcpServer(int threadNum, unsigned short port);
    void setListen(unsigned short port);
    void run();
    int acceptConn();

private:
    int m_threadNum;
    EventLoop *m_mainLoop;
    ThreadPool *m_pool;
    int m_lfd;
    unsigned short m_port;
};
