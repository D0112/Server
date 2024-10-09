# Server
该项目是为了学习Linux网络编程知识，是在Linux环境下使用C++语言开发的基于Reactor的轻量级多线程HTTP服务器，能够处理一定数量的客户端并发访问并及时响应，支持客户端访问服务器中的图片、音频、视频等资源。
## Channel模块
- 设置枚举类，方便后续修改回调事件
```
enum class FDtype
{
    timeOut = 0x01,
    ReadEvent = 0x02,
    WriteEvent = 0x04
};
```
- 将文件描述符fd包装起来，设置触发事件以及对应的回调函数，如读、写和销毁。设置void*类型指针，方便传参。
```
class Channel
{
public:
    //函数指针
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
```
- 初始化channel模块
```
//声明
Channel(int fd, FDtype events, handleFunc readFunc, handleFunc writeFunc, handleFunc destoryFunc, void *arg);
//实现
Channel::Channel(int fd, FDtype events, handleFunc readFunc, handleFunc writeFunc, handleFunc destoryFunc, void *arg)
{
    m_fd = fd;
    m_events = (int)events;
    m_readFunc = readFunc;
    m_writeFunc = writeFunc;
    m_destoryFunc = destoryFunc;
    m_arg = arg;
}
```
## Buffer模块
- 负责发送与接收数据，并将数据存储。
```
class Buffer
{
public:
    //初始化
    Buffer(int size);
    //回收资源
    ~Buffer();
    //扩容
    void bufferExtendRoom(int size);
    //获取可读的长度
    inline int readableSize()
    {
        return m_writePos - m_readPos;
    }
    //获取可写的长度
    inline int writeableSize()
    {
        return m_capacity - m_writePos;
    }
    //解析请求行时，获取分割字符串的起始位置
    inline char *getStart()
    {
        return m_data + m_readPos;
    }
    更新光标位置
    inline void readIncrease(int size)
    {
        m_readPos += size;
    }
    //写入数据到buffer
    int appendString(const char *data, int size);
      //传入字符数组
    int appendString(const char *data);
      //传入字符串
    int appendString(const string data);
    //读取fd中的数据到buffer
    int socketRead(int fd);
    //获取\r\n的位置
    char *findCRLF();
    //发送数据
    int sendData(int socket);

private:
    int m_capacity;
    char *m_data;
    int m_readPos = 0;
    int m_writePos = 0;
};
//实现
Buffer::Buffer(int size) : m_capacity(size)
{
    m_data = (char *)malloc(size);
    memset(m_data, 0, size);
}
Buffer::~Buffer()
{
    if (m_data != nullptr)
        free(m_data);
}

void Buffer::bufferExtendRoom(int size)
{
    int writeSize = writeableSize();
    if (writeSize >= size)
        return;
    //合并碎片化的空间
    else if (m_readPos + writeSize >= size)
    {
        int readSize = readableSize();
        memcpy(m_data, m_data + m_readPos, readSize);
        m_readPos = 0;
        m_writePos = readSize;
    }
    else
    {
        char *temp = (char *)realloc(m_data, m_capacity + size);
        if (temp == nullptr)
            return;
        memset(temp + m_capacity, 0, size);
        m_data = temp;
        m_capacity += size;
    }
}
int Buffer::appendString(const char *data, int size)
{
    assert(data == nullptr || size > 0);
    bufferExtendRoom(size);
    memcpy(m_data + m_writePos, data, size);
    m_writePos += size;
    return 0;
}

int Buffer::appendString(const string data)
{
    appendString(data.data());
    return 0;
}

int Buffer::appendString(const char *data)
{
    int size = strlen(data);
    appendString(data, size);
    return 0;
}

int Buffer::socketRead(int fd)
{
    int writeSize = writeableSize();
    struct iovec vec[2];
    vec[0].iov_base = m_data + m_writePos;
    vec[0].iov_len = writeSize;
    char *temp = (char *)malloc(40960);
    vec[1].iov_base = temp;
    vec[1].iov_len = 40960;
    int ret = readv(fd, vec, 2);
    if (ret == -1)
        return -1;
    else if (ret <= writeSize)
    {
        m_writePos += ret;
    }
    else
    {
        m_writePos = m_capacity;
        appendString(temp, ret - writeSize);
    }
    free(temp);
    return ret;
}

char *Buffer::findCRLF()
{
    char *ptr = (char *)memmem(m_data + m_readPos, readableSize(), "\r\n", 2);
    return ptr;
}

int Buffer::sendData(int socket)
{
    int readable = readableSize();
    if (readable > 0)
    {
        int count = send(socket, m_data + m_readPos, readable, MSG_NOSIGNAL);
        if (count > 0)
        {
            m_readPos += count;
            usleep(1);
        }
        return count;
    }
    return 0;
}
```
## Dispatcher模块
- I/O复用模型的父类
- 将需要子类重写的函数设置为虚函数
- 析构函数也设置为虚函数--调用子类的父类指针析构时也可调用子类的虚构函数
```
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
    // 事件检测分发，timeOut = 2表示事件检测只持续2s
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
```
- 子类-Epoll、Poll、Select
  - Epoll
    - 没有描述符个数限制
    - 不采取轮询的方式，只判断就绪链表是否为空，效率高。
    - 水平触发（LT）：默认工作模式，当 epoll_wait 检测到某描述符事件就绪并通知应用程序时，可以不立即处理，下次还会提醒。
    - 边缘触发（ET）：只提醒一次，需立即处理。
  ```
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
  //实现
  EpollDispatcher::EpollDispatcher(EventLoop *evLoop) : Dispatcher(evLoop)
  {
      // 创建epoll树
      m_epfd = epoll_create(1);//参数被废弃，大于0即可
      if (m_epfd == -1)
      {
          perror("epoll_create");
          exit(0);
      }
      //创建事件数组
      m_evs = new struct epoll_event[m_maxNode];
      m_name = "Epoll";
  }
  
  EpollDispatcher::~EpollDispatcher()
  {
      close(m_epfd);
      delete[] m_evs;
  }
  
  int EpollDispatcher::add()
  {
      int ret = EpollCtl(EPOLL_CTL_ADD);
      if (ret == -1)
      {
          perror("EpollAdd");
          exit(0);
      }
      return ret;
  }
  
  int EpollDispatcher::remove()
  {
      int ret = EpollCtl(EPOLL_CTL_DEL);
      if (ret == -1)
      {
          perror("EpollRemove");
          exit(0);
      }
      Debug("Epoll disconnection");
      //删除事件需要同时断开连接，回收资源
      m_channel->m_destoryFunc(const_cast<void *>(m_channel->getArg()));
      return ret;
  }
  
  int EpollDispatcher::modify()
  {
      int ret = EpollCtl(EPOLL_CTL_MOD);
      if (ret == -1)
      {
          perror("EpollModify");
          exit(0);
      }
      return ret;
  }
  
  int EpollDispatcher::dispatch(int timeOut)
  {
      //检测事件，阻塞态
      int count = epoll_wait(m_epfd, m_evs, m_maxNode, timeOut * 1000);
      //轮询所有事件
      for (int i = 0; i < count; ++i)
      {
          int fd = m_evs[i].data.fd;
          int events = m_evs[i].events;
          // 错误
          if (events & EPOLLERR || events & EPOLLHUP)
          {
              // 对方断开连接
              // EpollRemove(Channel, evLoop);
              continue;
          }
          //获取当前fd所对应的channel，其中存放着需要调用的回调函数
          if (events & EPOLLIN)
          {   //如果是读事件，则调用读回调
              m_evLoop->activate(fd, (int)FDtype::ReadEvent);
          }
          if (events & EPOLLOUT)
          {   //如果是写事件，则调用写回调
              m_evLoop->activate(fd, (int)FDtype::WriteEvent);
          }
      }
      return 0;
  }
  
  int EpollDispatcher::EpollCtl(int op)
  {
      int events = 0;
      if (m_channel->getEvents() & (int)FDtype::ReadEvent)
      {
          events |= EPOLLIN;
      }
      if ((m_channel->getEvents() & (int)FDtype::WriteEvent))
      {
          events |= EPOLLOUT;
      }
      epoll_event ev;
      ev.data.fd = m_channel->getSocket();
      ev.events = events;
      int ret = epoll_ctl(m_epfd, op, m_channel->getSocket(), &ev);
      return ret;
  }
  ```
  - Poll
    - 没有描述符个数限制。
    - 需要不断轮询所有描述符集合。
    - 每次调用都要将描述符集合从用户态往内核态拷贝一次。
  - Select
    - 描述符限制1024。
    - 需要不断轮询所有描述符集合。
    - 每次调用都要将描述符集合从用户态往内核态拷贝一次。
## EventLoop模块
```
// 枚举任务类型
enum class TaskType : char
{
    ADD,
    REMOVE,
    MODIFY
};
// 每个fd与任务类型之间的映射
class ChannelElem
{
public:
    Channel *channel;
    TaskType type;
};

class Dispatcher;
// 主线程与子线程分别维护一个EventLoop，前者负责监听有无客户端连接，后者负责处理新连接。
// 负责读写事件的分发。
// 该模块将调用Epoll、Poll、Select中的任意一个，实现I/O复用。
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
    // EventLoop的状态
    bool m_isQuit;
    Dispatcher *m_dispatcher;
    // 维护一个任务队列，对描述符进行添加、修改、删除操作。
    queue<ChannelElem *> m_TaskQ;
    // fd与存放该fd的channel映射
    map<int, Channel *> m_map;
    // 线程
    thread::id m_threadID;
    string m_threadName;
    mutex m_mutex;
    // 构建本地通讯对，防止所有子线程都阻塞在I/O检测时，无法解除阻塞。
    int m_socketPair[2];
};
```
```
//实现
//用于创建主线程的EventLoop
EventLoop::EventLoop() : EventLoop(string())
{
}

EventLoop::EventLoop(const string threadName)
{
    m_isQuit = true;
    // 选择Epoll
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
    //绑定函数
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
    {   // 检测事件
        m_dispatcher->dispatch();
        // 处理任务
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
```
## HttpRequest模块
- 解析Http请求，并将数据存放在该模块中。
- 将处理好的响应信息发送给客户端。
## HttpResponse模块
- 组织Http响应数据。
## TcpConnnection模块
- 调用HttpRequest模块与HttpResponse模块解析Http请求并组织响应数据。
- 本次连接的数据处理完断开连接，销毁已请求的空间
## ThreadWorker模块
- 主线程负责创建子线程。
- 子线程创建EventLoop实例。
- 利用线程同步，主线程等待子线程创建好EventLoop实例才可继续运行。
## ThreadPool模块
- 创建一定数量ThreadWorker实例。
- 从线程池中获取创建好EventLoop实例的子线程。
## TcpServer模块
- 创建监听描述符。
- 创建主线程的EventLoop模块，负责监听任务。
- 创建线程池实例。
- 获取子线程并创建子线程的EventLoop模块，负责处理新连接。
