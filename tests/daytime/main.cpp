#include "daytime.h"
#include "EventLoop.h"

#include <unistd.h>

int main() {

    printf("pid = %d\n", getpid());

    EventLoop loop;
    InetAddress listenAddr(2013);
    DaytimeServer server(&loop, listenAddr);
    server.start();
    loop.loop();
    return 0;
}