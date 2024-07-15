#pragma once
#include "EventLoop.h"
#include "Channel.h"
#include "Buffer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
using namespace std;
class TcpConnection
{
public:
    TcpConnection(EventLoop *evLoop, int fd);
    ~TcpConnection();
    static int destory(void *arg);
    int processRead();

private:
    string m_name;
    EventLoop *m_evLoop;
    Channel *m_channel;
    Buffer *m_readBuf;
    Buffer *m_writeBuf;
    // http协议
    HttpRequest *m_req;
    HttpResponse *m_res;
};
