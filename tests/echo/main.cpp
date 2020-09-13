#include "echo.h"

#include "EventLoop.h"

#include <unistd.h>
#include <stdio.h>

/**
 * 工作流程
 *  1. 建立一个事件循环器EventLoop
 *  2. 建立对应的业务服务器TcpServer
 *  3. 设置TcpServer的Callback
 *  4. 启动server
 *  5. 开启事件循环
 */
int main() {

    printf("pid = %d\n", getpid());
    EventLoop loop;
    InetAddress listenAddr(2007);
    EchoServer server(&loop, listenAddr);

    server.start();
    loop.loop();

    return 0;
}