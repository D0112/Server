#pragma once
#include "EventLoop.h"
#include "Dispatcher.h"
#include <sys/epoll.h>
using namespace std;
class EpollDispatcher : public Dispatcher
{
private:
    int m_epfd;
    epoll_event *m_evs;
    const int m_maxNode = 520;

private:
    int EpollCtl(int op);

public:
    EpollDispatcher(EventLoop *evLoop);
    ~EpollDispatcher();
    int add() override;
    int remove() override;
    int modify() override;
    int dispatch(int timeOut = 2) override;
};
