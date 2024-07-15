#pragma once
#include <EventLoop.h>
#include <condition_variable>
class ThreadWorker
{
public:
    ThreadWorker(int index);
    ~ThreadWorker();
    void run();
    inline EventLoop *getEventLoop()
    {
        return m_evLoop;
    }

private:
    void running();

private:
    thread *m_thread;
    thread::id m_threadID;
    string m_threadName;
    EventLoop *m_evLoop;
    mutex m_mutex;
    condition_variable m_cond;
};
