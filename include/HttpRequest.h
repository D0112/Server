#pragma once
#include <map>
#include <stdlib.h>
#include <strings.h>
#include "Buffer.h"
#include <assert.h>
#include <ctype.h>
#include <sys/stat.h>
#include "HttpResponse.h"
#include <functional>

using namespace std;

enum class processState : char
{
    parseRequestLine,
    parseRequestHeaders,
    parseRequestBody,
    parseRequestDone
};

class HttpRequest
{
public:
    HttpRequest();
    ~HttpRequest();
    // 重置
    void reset();
    // 存储请求头
    void addHeader(const string key, const string value);
    // 根据key取出value
    string getHeader(const string key);
    // 解析请求行
    bool parseLine(Buffer *readBuf);
    // 解析请求头
    bool parseHeader(Buffer *readBuf);
    // 解析请求协议
    bool parseHttpRequest(Buffer *readBuf, HttpResponse *res, Buffer *sendBuf, int socket);
    // 处理请求协议
    bool processHttpRequest(HttpResponse *res);
    // 解码
    string decodeMsg(string msg);
    const string getFileType(const string name);

    static void sendFile(const string fileName, Buffer *sendBuf, int cfd);
    static void sendDir(const string dirName, Buffer *sendBuf, int cfd);

    inline void setState(processState state)
    {
        m_curState = state;
    }
    inline void setMethod(string method)
    {
        m_method = method;
    }
    inline void setUrl(string url)
    {
        m_url = url;
    }
    inline void setVersion(string version)
    {
        m_version = version;
    }

private:
    char *splitRequestLine(const char *start, const char *end,
                           const char *sub, function<void(string)> callback);
    int hexToDec(char c);

private:
    string m_method;
    string m_url;
    string m_version;
    map<string, string> m_reqHeaders;
    processState m_curState;
};
