#pragma once
#include <stdlib.h>
#include <functional>
using namespace std;
enum class FDtype
{
    timeOut = 0x01,
    ReadEvent = 0x02,
    WriteEvent = 0x04
};
class Channel
{
public:
    using handleFunc = function<int(void *)>;
    Channel(int fd, FDtype events, handleFunc readFunc, handleFunc writeFunc, handleFunc destoryFunc, void *arg);
    handleFunc m_readFunc;
    handleFunc m_writeFunc;
    handleFunc m_destoryFunc;
    int WriteEnable(int flag);
    bool isWriteEnable();
    inline int getSocket()
    {
        return m_fd;
    }
    inline int getEvents()
    {
        return m_events;
    }
    inline const void *getArg()
    {
        return m_arg;
    }

private:
    int m_fd;
    int m_events;
    void *m_arg;
};
