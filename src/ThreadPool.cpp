#include "ThreadPool.h"
#include <assert.h>

ThreadPool::ThreadPool(EventLoop *mainLoop, int count)
{
    m_mainLoop = mainLoop;
    m_isStart = false;
    m_threadNum = count;
    m_index = 0;
    m_threadWorkers.clear();
}

void ThreadPool::run()
{
    assert(!m_isStart);
    if (m_mainLoop->getThreadID() != this_thread::get_id())
    {
        perror("threadPoolRun");
        exit(0);
    }
    m_isStart = true;
    if (m_threadNum > 0)
    {
        for (int i = 0; i < m_threadNum; ++i)
        {
            ThreadWorker *worker = new ThreadWorker(i);
            worker->run();
            m_threadWorkers.push_back(worker);
        }
    }
}

EventLoop *ThreadPool::takeWorkerEventLoop()
{
    assert(m_isStart);
    if (m_mainLoop->getThreadID() != this_thread::get_id())
    {
        perror("threadPoolRun");
        exit(0);
    }
    EventLoop *evLoop = m_mainLoop;
    if (m_threadNum > 0)
    {
        evLoop = m_threadWorkers[m_index]->getEventLoop();
        m_index = ++m_index % m_threadNum;
    }
    return evLoop;
}
