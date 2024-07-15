#include "TcpServer.h"

int main(int argc, char *argv[])
{
    // 设置传入参数
    // path为服务器工作路径，该路径下保存客户端可访问的文件
    if (argc < 3)
    {
        perror("./a.out port path\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);
    // 切换到工作目录
    chdir(argv[2]);
    // 初始化监听socket
    auto server = new TcpServer(4, port);
    // 启动服务器
    server->run();
    return 0;
}