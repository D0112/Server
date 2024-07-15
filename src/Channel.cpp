#include "Channel.h"
Channel::Channel(int fd, FDtype events, handleFunc readFunc, handleFunc writeFunc, handleFunc destoryFunc, void *arg)
{
    m_fd = fd;
    m_events = (int)events;
    m_readFunc = readFunc;
    m_writeFunc = writeFunc;
    m_destoryFunc = destoryFunc;
    m_arg = arg;
}

int Channel::WriteEnable(int flag)
{
    if (flag)
    {
        m_events |= static_cast<int>(FDtype::ReadEvent);
    }
    else
    {
        m_events = m_events & ~static_cast<int>(FDtype::WriteEvent);
    }
    return 0;
}

bool Channel::isWriteEnable()
{
    return m_events & static_cast<int>(FDtype::WriteEvent);
}
