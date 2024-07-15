#include "HttpRequest.h"
HttpRequest::HttpRequest()
{
    reset();
}

HttpRequest::~HttpRequest()
{
}

void HttpRequest::reset()
{
    m_method = string();
    m_url = string();
    m_version = string();
    m_reqHeaders.clear();
    m_curState = processState::parseRequestLine;
}

void HttpRequest::addHeader(const string key, const string value)
{
    if (key.empty() || value.empty())
    {
        return;
    }
    m_reqHeaders.insert(make_pair(key, value));
}

string HttpRequest::getHeader(const string key)
{
    auto item = m_reqHeaders.find(key);
    if (item == m_reqHeaders.end())
    {
        return string();
    }
    return item->second;
}

bool HttpRequest::parseLine(Buffer *readBuf)
{
    char *end = readBuf->findCRLF();
    char *start = readBuf->getStart();
    int lineSize = end - start;
    if (lineSize > 0)
    {
        // 解析method
        auto MethodFunc = bind(&HttpRequest::setMethod, this, placeholders::_1);
        start = splitRequestLine(start, end, " ", MethodFunc);
        // 解析url
        auto UrlFunc = bind(&HttpRequest::setUrl, this, placeholders::_1);
        start = splitRequestLine(start, end, " ", UrlFunc);
        // 解析version
        auto VersionFunc = bind(&HttpRequest::setVersion, this, placeholders::_1);
        splitRequestLine(start, end, nullptr, VersionFunc);
        // 转到下一行
        readBuf->readIncrease(lineSize + 2);
        // 修改状态
        setState(processState::parseRequestHeaders);
        return true;
    }
    return false;
}

bool HttpRequest::parseHeader(Buffer *readBuf)
{
    char *end = readBuf->findCRLF();
    if (end != nullptr)
    {
        char *start = readBuf->getStart();
        int lineSize = end - start;
        char *middle = (char *)memmem(start, lineSize, ": ", 2);
        if (middle != nullptr)
        {
            int keyLen = middle - start;
            int valueLen = end - middle - 2;
            if (keyLen > 0 && valueLen > 0)
            {
                string key = string(start, keyLen);
                string value = string(middle + 2, valueLen);
                addHeader(key, value);
            }

            // 转到下一行
            readBuf->readIncrease(lineSize + 2);
        }
        else
        {
            // 解析完毕，转到下一行
            readBuf->readIncrease(2);
            // 修改状态
            setState(processState::parseRequestDone);
        }
        return true;
    }
    return false;
}

bool HttpRequest::parseHttpRequest(Buffer *readBuf, HttpResponse *res, Buffer *sendBuf, int socket)
{
    bool flag = true;
    while (m_curState != processState::parseRequestDone)
    {
        switch (m_curState)
        {
        case processState::parseRequestLine:
            flag = parseLine(readBuf);
            break;
        case processState::parseRequestHeaders:
            flag = parseHeader(readBuf);
            break;
        case processState::parseRequestBody:
            break;
        default:
            break;
        }
        if (!flag)
            return flag;
        if (m_curState == processState::parseRequestDone)
        {
            // 处理解析请求
            processHttpRequest(res);
            // 组织响应数据
            res->prepareMsg(sendBuf, socket);
        }
    }
    // 确保可以解析后续请求
    m_curState = processState::parseRequestLine;
    return flag;
}

bool HttpRequest::processHttpRequest(HttpResponse *res)
{
    // 判断字符串是否相等
    if (strcasecmp(m_method.data(), "get") != 0)
    {
        // 不相等则退出
        return false;
    }
    // 解码
    m_url = decodeMsg(m_url);
    // 处理客户端请求的静态资源（目录或文件）
    // 判断是否为当前服务器的工作目录
    const char *file = nullptr;
    if (strcmp(m_url.data(), "/") == 0)
    {
        file = (char *)"./";
    }
    else
    {
        // 设置相对路径
        file = m_url.data() + 1;
    }
    // 判断该路径是文件还是目录
    struct stat st;
    // 获取文件信息
    int ret = stat(file, &st);
    if (ret == -1)
    {
        // 文件不存在 404
        // sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
        // sendFile("404.html", cfd);
        res->setFilename("404.html");
        res->setStatus(StatusCode::NotFound);
        res->addHeader("Content-type", getFileType(".html"));
        res->sendDataFunc = sendFile;
        return 0;
    }
    res->setFilename(file);
    res->setStatus(StatusCode::OK);
    if (S_ISDIR(st.st_mode))
    {
        // 为目录，将目录信息返回客户端
        // sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
        // sendDir(file, cfd);
        res->addHeader("Content-type", getFileType(".html"));
        res->sendDataFunc = sendDir;
    }
    else
    {
        // 为文件，将文件返回客户端
        // sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
        // sendFile(file, cfd);
        res->addHeader("Content-type", getFileType(file));
        res->addHeader("Content-length", to_string(st.st_size));
        res->sendDataFunc = sendFile;
    }
    return false;
}

string HttpRequest::decodeMsg(string msg)
{
    string str = string();
    const char *from = msg.data();
    for (; *from != '\0'; ++from)
    {
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            str.append(1, hexToDec(from[1]) * 16 + hexToDec(from[2]));
            from += 2;
        }
        else
        {
            str.append(1, *from);
        }
    }
    str.append(1, '\0');
    return str;
}

const string HttpRequest::getFileType(const string name)
{
    // a.jpg a.mp4 a.html
    // 从右向左查找“.”，不存在则返回NULL
    const char *dot = strrchr(name.data(), '.');
    if (dot == NULL)
        return "text/plain; charset=utf-8"; // 纯文本
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";
    return "text/plain; charset=utf-8";
}

void HttpRequest::sendFile(const string fileName, Buffer *sendBuf, int cfd)
{
    // 打开文件
    int ffd = open(fileName.data(), O_RDONLY);
    // 断言文件是否打开成功
    assert(ffd > 0);
    while (1)
    {
        char buf[1024];
        int len = read(ffd, buf, sizeof buf);
        if (len > 0)
        {
            // send(cfd, buf, len, 0);
            sendBuf->appendString(buf, len);
            sendBuf->sendData(cfd);
        }
        else if (len == 0)
        {
            break;
        }
        else
        {
            close(ffd);
            perror("read");
        }
    }
    close(ffd);
}

void HttpRequest::sendDir(const string dirName, Buffer *sendBuf, int cfd)
{
    // 将目录以html的形式发给客户端，因此需要申请内存来保存html格式的文件
    char buff[4096] = {0};
    sprintf(buff, "<html><head><title>%s</title></head><body><table>", dirName.data());
    //  获得当前目录下的文件名与数量
    //  指针数组，数组内为指针
    struct dirent **namelist;
    int num = scandir(dirName.data(), &namelist, NULL, alphasort);
    // 遍历每个文件
    for (int i = 0; i < num; ++i)
    {
        // 当前文件名
        char *name = namelist[i]->d_name;
        // 获得相对路径
        char subpath[1024] = {0};
        if (dirName.back() == '/')
        {
            sprintf(subpath, "%s%s", dirName.data(), name);
        }
        else
        {
            sprintf(subpath, "%s/%s", dirName.data(), name);
        }
        // 获得文件属性
        struct stat st;
        stat(subpath, &st);
        if (S_ISDIR(st.st_mode))
        {
            // 如果为目录
            // a标签 <a href="">name</a> 跳转
            sprintf(buff + strlen(buff),
                    "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
                    name, name, st.st_size);
        }
        else
        {
            // 如果为文件
            sprintf(buff + strlen(buff),
                    "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
                    name, name, st.st_size);
        }
        // 发送
        sendBuf->appendString(buff);
        sendBuf->sendData(cfd);
        // 清空缓存
        memset(buff, 0, sizeof(buff));
        // 释放指针
        free(namelist[i]);
    }
    // 补充后续html
    sprintf(buff, "</table></body></html>");
    sendBuf->appendString(buff);
    sendBuf->sendData(cfd);
    // 释放数组指针
    free(namelist);
}

char *HttpRequest::splitRequestLine(const char *start, const char *end, const char *sub, function<void(string)> callback)
{
    char *space = (char *)end;
    if (sub != nullptr)
    {
        space = (char *)memmem(start, end - start, sub, strlen(sub));
        assert(space != nullptr);
    }
    callback(string(start, space - start));
    return space + 1;
}

int HttpRequest::hexToDec(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0;
}
