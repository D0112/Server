#include "EpollDispatcher.h"
#include "Log.h"
EpollDispatcher::EpollDispatcher(EventLoop *evLoop) : Dispatcher(evLoop)
{
    // 创建epoll树
    m_epfd = epoll_create(1);
    if (m_epfd == -1)
    {
        perror("epoll_create");
        exit(0);
    }
    m_evs = new struct epoll_event[m_maxNode];
    m_name = "Epoll";
}

EpollDispatcher::~EpollDispatcher()
{
    close(m_epfd);
    delete[] m_evs;
}

int EpollDispatcher::add()
{
    int ret = EpollCtl(EPOLL_CTL_ADD);
    if (ret == -1)
    {
        perror("EpollAdd");
        exit(0);
    }
    return ret;
}

int EpollDispatcher::remove()
{
    int ret = EpollCtl(EPOLL_CTL_DEL);
    if (ret == -1)
    {
        perror("EpollRemove");
        exit(0);
    }
    Debug("Epoll disconnection");
    m_channel->m_destoryFunc(const_cast<void *>(m_channel->getArg()));
    return ret;
}

int EpollDispatcher::modify()
{
    int ret = EpollCtl(EPOLL_CTL_MOD);
    if (ret == -1)
    {
        perror("EpollModify");
        exit(0);
    }
    return ret;
}

int EpollDispatcher::dispatch(int timeOut)
{
    int count = epoll_wait(m_epfd, m_evs, m_maxNode, timeOut * 1000);
    for (int i = 0; i < count; ++i)
    {
        int fd = m_evs[i].data.fd;
        int events = m_evs[i].events;
        // 错误
        if (events & EPOLLERR || events & EPOLLHUP)
        {
            // 对方断开连接
            // EpollRemove(Channel, evLoop);
            continue;
        }
        if (events & EPOLLIN)
        {
            m_evLoop->activate(fd, (int)FDtype::ReadEvent);
        }
        if (events & EPOLLOUT)
        {
            m_evLoop->activate(fd, (int)FDtype::WriteEvent);
        }
    }
    return 0;
}

int EpollDispatcher::EpollCtl(int op)
{
    int events = 0;
    if (m_channel->getEvents() & (int)FDtype::ReadEvent)
    {
        events |= EPOLLIN;
    }
    if ((m_channel->getEvents() & (int)FDtype::WriteEvent))
    {
        events |= EPOLLOUT;
    }
    epoll_event ev;
    ev.data.fd = m_channel->getSocket();
    ev.events = events;
    int ret = epoll_ctl(m_epfd, op, m_channel->getSocket(), &ev);
    return ret;
}
