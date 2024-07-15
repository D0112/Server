#pragma once
#include <stdlib.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <string>
using namespace std;
class Buffer
{
public:
    Buffer(int size);
    ~Buffer();
    void bufferExtendRoom(int size);
    inline int readableSize()
    {
        return m_writePos - m_readPos;
    }
    inline int writeableSize()
    {
        return m_capacity - m_writePos;
    }
    inline char *getStart()
    {
        return m_data + m_readPos;
    }
    inline void readIncrease(int size)
    {
        m_readPos += size;
    }
    int appendString(const char *data, int size);
    int appendString(const char *data);
    int appendString(const string data);
    int socketRead(int fd);
    char *findCRLF();
    int sendData(int socket);

private:
    int m_capacity;
    char *m_data;
    int m_readPos = 0;
    int m_writePos = 0;
};
