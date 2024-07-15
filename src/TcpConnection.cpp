#include "TcpConnection.h"
#include "Log.h"
int TcpConnection::processRead()
{
    int socket = m_channel->getSocket();
    int count = m_readBuf->socketRead(socket);
    Debug("接收到的http请求数据: %s", m_readBuf->getStart());
    if (count > 0)
    {
        // 解析请求
        bool flag = m_req->parseHttpRequest(m_readBuf, m_res, m_writeBuf, socket);
        if (!flag)
        {
            // 解析错误
            char *errMsg = "Http/1.1 400 BadMsg\r\n\r\n";
            m_writeBuf->appendString(errMsg);
            m_writeBuf->sendData(socket);
        }
    }
    // 断开连接
    Debug("断开连接");
    m_evLoop->addTask(m_channel, TaskType::REMOVE);
    return 0;
}

TcpConnection::TcpConnection(EventLoop *evLoop, int fd)
{
    m_evLoop = evLoop;
    m_readBuf = new Buffer(10240);
    m_writeBuf = new Buffer(10240);
    m_req = new HttpRequest();
    m_res = new HttpResponse();
    m_name = "Connection-" + to_string(fd);
    auto obj = bind(&TcpConnection::processRead, this);
    m_channel = new Channel(fd, FDtype::ReadEvent, obj, nullptr, destory, this);
    evLoop->addTask(m_channel, TaskType::ADD);
}

TcpConnection::~TcpConnection()
{
    if (m_readBuf && m_readBuf->readableSize() == 0 && m_writeBuf && m_writeBuf->readableSize() == 0)
    {
        delete m_readBuf;
        delete m_writeBuf;
        delete m_req;
        delete m_res;
        m_evLoop->destoryChannel(m_channel);
    }
    Debug("连接断开, 释放资源, gameover, connName: %s", m_name.data());
}

int TcpConnection::destory(void *arg)
{
    TcpConnection *conn = static_cast<TcpConnection *>(arg);
    if (conn != nullptr)
    {
        delete conn;
    }
    return 0;
}
