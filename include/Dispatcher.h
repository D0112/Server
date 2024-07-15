#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
using namespace std;
class EventLoop;
class Dispatcher
{
public:
    Dispatcher(EventLoop *evLoop);
    virtual ~Dispatcher();
    // 添加
    virtual int add();
    // 删除
    virtual int remove();
    // 修改
    virtual int modify();
    // 事件检测分发
    virtual int dispatch(int timeOut = 2);
    inline void setChannel(Channel *channel)
    {
        m_channel = channel;
    }

protected:
    string m_name = string();
    Channel *m_channel;
    EventLoop *m_evLoop;
};
