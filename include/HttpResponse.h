#pragma once
#include "Buffer.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <map>
#include <functional>

using namespace std;
enum StatusCode
{
    Unknown,
    OK = 200,
    MovedPermanently = 301,
    MovedTemporarily = 302,
    BadRequest = 400,
    NotFound = 404
};

class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();
    function<void(const string, Buffer *, int)> sendDataFunc;
    // 添加响应头
    void addHeader(const string key, const string value);
    // 组织响应数据
    void prepareMsg(Buffer *sendBuf, int socket);
    inline void setFilename(const string name)
    {
        m_fileName = name;
    }
    inline void setStatus(StatusCode status)
    {
        m_status = status;
    }

private:
    const map<int, string> m_info = {
        {0, "Unknown"},
        {200, "OK"},
        {301, "MovedPermanently"},
        {302, "MovedTemporarily"},
        {400, "BadRequest"},
        {404, "NotFound"},
    };
    StatusCode m_status;
    map<string, string> m_resHeaders;
    string m_fileName;
};
