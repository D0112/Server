#include "HttpResponse.h"

HttpResponse::HttpResponse()
{
    m_status = StatusCode::Unknown;
    m_resHeaders.clear();
    m_fileName = string();
    sendDataFunc = nullptr;
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::addHeader(const string key, const string value)
{
    if (key.empty() || value.empty())
    {
        return;
    }
    m_resHeaders.insert(make_pair(key, value));
}

void HttpResponse::prepareMsg(Buffer *sendBuf, int socket)
{
    // 拼接状态行
    char temp[1024] = {0};
    int code = static_cast<int>(m_status);
    sprintf(temp, "Http/1.1 %d %s\r\n", code, m_info.at(code).data());
    sendBuf->appendString(temp);
    // 拼接响应头
    for (auto item : m_resHeaders)
    {
        sprintf(temp, "%s :%s\r\n", item.first.data(), item.second.data());
        sendBuf->appendString(temp);
    }
    sendBuf->appendString("\r\n");
    sendBuf->sendData(socket);
    // 发送数据
    sendDataFunc(m_fileName, sendBuf, socket);
}
