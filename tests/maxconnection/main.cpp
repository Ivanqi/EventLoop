#include "echo.h"
#include "EventLoop.h"

#include <unistd.h>

int main(int argc, char* argv[]) {

    printf("pid = %d\n", getpid());
    EventLoop loop;
    InetAddress listenAddr(2007);
    int maxConnections = 5;

    if (argc > 1) {
        maxConnections = atoi(argv[1]);
    }

    printf("maxConnections = %d\n", maxConnections);
    EchoServer server(&loop, listenAddr, maxConnections);
    server.start();
    loop.loop();

    return 0;
}