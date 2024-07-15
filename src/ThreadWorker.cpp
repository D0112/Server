#include "ThreadWorker.h"

ThreadWorker::ThreadWorker(int index)
{
    m_thread = nullptr;
    m_evLoop = nullptr;
    m_threadID = thread::id();
    m_threadName = "SubThread-" + to_string(index);
}

ThreadWorker::~ThreadWorker()
{
    if (m_thread != nullptr)
        delete m_thread;
}

void ThreadWorker::running()
{
    m_mutex.lock();
    m_evLoop = new EventLoop(m_threadName);
    m_mutex.unlock();
    m_cond.notify_one();
    m_evLoop->run();
}

void ThreadWorker::run()
{
    m_thread = new thread(&ThreadWorker::running, this);
    unique_lock<mutex> locker(m_mutex);
    while (m_evLoop == nullptr)
    {
        m_cond.wait(locker);
    }
}
