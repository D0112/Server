#include "Buffer.h"
#include <assert.h>

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
