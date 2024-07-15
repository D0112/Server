#include "EventLoop.h"
#include "EpollDispatcher.h"

EventLoop::EventLoop() : EventLoop(string())
{
}

EventLoop::EventLoop(const string threadName)
{
    m_isQuit = true;
    m_dispatcher = new EpollDispatcher(this);
    m_map.clear();
    m_threadID = this_thread::get_id();
    m_threadName = threadName == string() ? "MainThread" : threadName;
    // 创建本地通讯套接字，socketPair[0]:发送数据，socketPair[1]：接收数据
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketPair);
    if (ret == -1)
    {
        perror("socketpair");
        exit(0);
    }
    auto obj = bind(&EventLoop::readLocalMsg, this);
    Channel *channel = new Channel(m_socketPair[1], FDtype::ReadEvent,
                                   obj, nullptr, nullptr, this);
    addTask(channel, TaskType::ADD);
}

EventLoop::~EventLoop()
{
}

int EventLoop::run()
{
    m_isQuit = false;
    // 判断线程是否一致
    if (m_threadID != this_thread::get_id())
        return -1;
    while (!m_isQuit)
    {
        m_dispatcher->dispatch();
        processTaskQ();
    }
    return 0;
}

int EventLoop::activate(int fd, int events)
{
    if (fd < 0)
        return -1;
    // 取出文件描述符对应的channel
    Channel *channel = m_map[fd];
    // 确保fd一致
    assert(channel->getSocket() == fd);
    if (events & (int)FDtype::ReadEvent && channel->m_readFunc)
    {
        channel->m_readFunc(const_cast<void *>(channel->getArg()));
    }
    if (events & (int)FDtype::WriteEvent && channel->m_writeFunc)
    {
        channel->m_writeFunc(const_cast<void *>(channel->getArg()));
    }
    return 0;
}

int EventLoop::addTask(Channel *channel, TaskType type)
{
    m_mutex.lock();
    // 创建任务节点
    ChannelElem *node = new ChannelElem;
    node->channel = channel;
    node->type = type;
    // 添加任务
    m_TaskQ.push(node);
    m_mutex.unlock();

    // 处理节点
    if (m_threadID == this_thread::get_id())
    {
        // 当前子线程
        processTaskQ();
    }
    else
    {
        // 主线程,加入唤醒通信
        writeLocalMsg();
    }
    return 0;
}

int EventLoop::processTaskQ()
{

    while (!m_TaskQ.empty())
    {
        m_mutex.lock();
        // 取出任务队列头节点
        ChannelElem *head = m_TaskQ.front();
        m_TaskQ.pop();
        m_mutex.unlock();
        Channel *channel = head->channel;
        if (head->type == TaskType::ADD)
        {
            // 添加
            add(channel);
        }
        else if (head->type == TaskType::REMOVE)
        {
            // 删除
            remove(channel);
        }
        else if (head->type == TaskType::MODIFY)
        {
            // 修改
            modify(channel);
        }
        delete head;
    }
    return 0;
}

int EventLoop::add(Channel *channel)
{
    int fd = channel->getSocket();
    if (m_map.find(fd) == m_map.end())
    {
        m_map.insert(make_pair(fd, channel));
        m_dispatcher->setChannel(channel);
        int ret = m_dispatcher->add();
        return ret;
    }
    return -1;
}

int EventLoop::remove(Channel *channel)
{
    int fd = channel->getSocket();
    if (m_map.find(fd) == m_map.end())
    {
        return -1;
    }
    m_dispatcher->setChannel(channel);
    int ret = m_dispatcher->remove();
    return ret;
}

int EventLoop::modify(Channel *channel)
{
    int fd = channel->getSocket();
    if (m_map.find(fd) == m_map.end())
    {
        return -1;
    }
    m_dispatcher->setChannel(channel);
    int ret = m_dispatcher->modify();
    return ret;
}

int EventLoop::destoryChannel(Channel *channel)
{
    // 解除channel与fd的关系
    auto it = m_map.find(channel->getSocket());
    if (it != m_map.end())
    {
        m_map.erase(it);
        close(channel->getSocket());
        delete channel;
    }
    return 0;
}

int EventLoop::readLocalMsg()
{
    char buf[256];
    read(m_socketPair[1], buf, sizeof(buf));
    return 0;
}

void EventLoop::writeLocalMsg()
{
    const char *msg = "111";
    write(m_socketPair[0], msg, strlen(msg));
}
