#include "echo.h"
#include <stdio.h>

#include "EventLoop.h"

int main(int argc, char* argv[]) {

    EventLoop loop;
    InetAddress listenAddr(2007);
    int idleSeconds = 10;

    if (argc > 1) {
        idleSeconds = atoi(argv[1]);
    }

    printf("pid = %d, idle seconds = %d\n", getpid(), idleSeconds);
    EchoServer server(&loop, listenAddr, idleSeconds);
    server.start();

    loop.loop();

    return 0;
}