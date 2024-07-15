#pragma once
#include "Dispatcher.h"
#include <assert.h>
#include <sys/socket.h>
#include <queue>
#include <map>
#include <thread>
#include <mutex>
#include <string.h>
using namespace std;
enum class TaskType : char
{
    ADD,
    REMOVE,
    MODIFY
};

class ChannelElem
{
public:
    Channel *channel;
    TaskType type;
};

class Dispatcher;

class EventLoop
{
public:
    EventLoop();
    EventLoop(const string threadName);
    ~EventLoop();

    // 启动反应堆模型
    int run();
    // 处理被激活的文件描述符
    int activate(int fd, int events);
    // 向任务队列添加任务
    int addTask(Channel *channel, TaskType type);
    // 处理dispatcher中的节点
    int processTaskQ();
    int add(Channel *channel);
    int remove(Channel *channel);
    int modify(Channel *channel);
    int destoryChannel(Channel *channel);
    int readLocalMsg();
    void writeLocalMsg();
    inline thread::id getThreadID()
    {
        return m_threadID;
    }
    inline string getThreadName()
    {
        return m_threadName;
    }

private:
    bool m_isQuit;
    Dispatcher *m_dispatcher;
    // 任务队列
    queue<ChannelElem *> m_TaskQ;
    // map
    map<int, Channel *> m_map;
    // 线程
    thread::id m_threadID;
    string m_threadName;
    mutex m_mutex;
    int m_socketPair[2];
};
