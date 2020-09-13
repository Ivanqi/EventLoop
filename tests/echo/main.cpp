#include "echo.h"

#include "EventLoop.h"

#include <unistd.h>
#include <stdio.h>

int main() {

    printf("pid = %d\n", getpid());
    EventLoop loop;
    InetAddress listenAddr(2007);
    EchoServer server(&loop, listenAddr);

    server.start();
    loop.loop();

    return 0;
}