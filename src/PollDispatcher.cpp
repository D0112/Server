#include "Dispatcher.h"
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include "PollDispatcher.h"

PollDispatcher::PollDispatcher(EventLoop *evloop) : Dispatcher(evloop)
{
    m_maxfd = 0;
    m_fds = new struct pollfd[m_maxNode];
    for (int i = 0; i < m_maxNode; ++i)
    {
        m_fds[i].fd = -1;
        m_fds[i].events = 0;
        m_fds[i].revents = 0;
    }
    m_name = "Poll";
}

PollDispatcher::~PollDispatcher()
{
    delete[] m_fds;
}

int PollDispatcher::add()
{
    int events = 0;
    if (m_channel->getEvents() & (int)FDtype::ReadEvent)
    {
        events |= POLLIN;
    }
    if (m_channel->getEvents() & (int)FDtype::WriteEvent)
    {
        events |= POLLOUT;
    }
    int i = 0;
    for (; i < m_maxNode; ++i)
    {
        if (m_fds[i].fd == -1)
        {
            m_fds[i].events = events;
            m_fds[i].fd = m_channel->getSocket();
            m_maxfd = i > m_maxfd ? i : m_maxfd;
            break;
        }
    }
    if (i >= m_maxNode)
    {
        return -1;
    }
    return 0;
}

int PollDispatcher::remove()
{
    int i = 0;
    for (; i < m_maxNode; ++i)
    {
        if (m_fds[i].fd == m_channel->getSocket())
        {
            m_fds[i].events = 0;
            m_fds[i].revents = 0;
            m_fds[i].fd = -1;
            break;
        }
    }
    // 通过 channel 释放对应的 TcpConnection 资源
    m_channel->m_destoryFunc(const_cast<void *>(m_channel->getArg()));
    if (i >= m_maxNode)
    {
        return -1;
    }
    return 0;
}

int PollDispatcher::modify()
{
    int events = 0;
    if (m_channel->getEvents() & (int)FDtype::ReadEvent)
    {
        events |= POLLIN;
    }
    if (m_channel->getEvents() & (int)FDtype::WriteEvent)
    {
        events |= POLLOUT;
    }
    int i = 0;
    for (; i < m_maxNode; ++i)
    {
        if (m_fds[i].fd == m_channel->getSocket())
        {
            m_fds[i].events = events;
            break;
        }
    }
    if (i >= m_maxNode)
    {
        return -1;
    }
    return 0;
}

int PollDispatcher::dispatch(int timeout)
{
    int count = poll(m_fds, m_maxfd + 1, timeout * 1000);
    if (count == -1)
    {
        perror("poll");
        exit(0);
    }
    for (int i = 0; i <= m_maxfd; ++i)
    {
        if (m_fds[i].fd == -1)
        {
            continue;
        }

        if (m_fds[i].revents & POLLIN)
        {
            m_evLoop->activate(m_fds[i].fd, (int)FDtype::ReadEvent);
        }
        if (m_fds[i].revents & POLLOUT)
        {
            m_evLoop->activate(m_fds[i].fd, (int)FDtype::WriteEvent);
        }
    }
    return 0;
}
