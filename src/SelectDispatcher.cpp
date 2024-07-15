#include "Dispatcher.h"
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include "SelectDispatcher.h"

SelectDispatcher::SelectDispatcher(EventLoop *evloop) : Dispatcher(evloop)
{
    FD_ZERO(&m_readSet);
    FD_ZERO(&m_writeSet);
    m_name = "Select";
}

SelectDispatcher::~SelectDispatcher()
{
}

int SelectDispatcher::add()
{
    if (m_channel->getSocket() >= m_maxSize)
    {
        return -1;
    }
    setFdSet();
    return 0;
}

int SelectDispatcher::remove()
{
    clearFdSet();
    // 通过 channel 释放对应的 TcpConnection 资源
    m_channel->m_destoryFunc(const_cast<void *>(m_channel->getArg()));

    return 0;
}

int SelectDispatcher::modify()
{
    setFdSet();
    clearFdSet();
    return 0;
}

int SelectDispatcher::dispatch(int timeout)
{
    struct timeval val;
    val.tv_sec = timeout;
    val.tv_usec = 0;
    fd_set rdtmp = m_readSet;
    fd_set wrtmp = m_writeSet;
    int count = select(m_maxSize, &rdtmp, &wrtmp, NULL, &val);
    if (count == -1)
    {
        perror("select");
        exit(0);
    }
    for (int i = 0; i < m_maxSize; ++i)
    {
        if (FD_ISSET(i, &rdtmp))
        {
            m_evLoop->activate(i, (int)FDtype::ReadEvent);
        }

        if (FD_ISSET(i, &wrtmp))
        {
            m_evLoop->activate(i, (int)FDtype::WriteEvent);
        }
    }
    return 0;
}

void SelectDispatcher::setFdSet()
{
    if (m_channel->getEvents() & (int)FDtype::ReadEvent)
    {
        FD_SET(m_channel->getSocket(), &m_readSet);
    }
    if (m_channel->getEvents() & (int)FDtype::WriteEvent)
    {
        FD_SET(m_channel->getSocket(), &m_writeSet);
    }
}

void SelectDispatcher::clearFdSet()
{
    if (m_channel->getEvents() & (int)FDtype::ReadEvent)
    {
        FD_CLR(m_channel->getSocket(), &m_readSet);
    }
    if (m_channel->getEvents() & (int)FDtype::WriteEvent)
    {
        FD_CLR(m_channel->getSocket(), &m_writeSet);
    }
}
