#pragma once
#include "ThreadWorker.h"
#include <vector>
using namespace std;
class ThreadPool
{
public:
    ThreadPool(EventLoop *mainLoop, int count);
    ~ThreadPool();
    void run();
    EventLoop *takeWorkerEventLoop();

private:
    EventLoop *m_mainLoop;
    bool m_isStart;
    int m_index;
    int m_threadNum;
    vector<ThreadWorker *> m_threadWorkers;
};
