#include "chargen.h"
#include "EventLoop.h"
#include <unistd.h>

int main() {

    printf("pid = %d\n", getpid());
    EventLoop loop;
    InetAddress listenAddr(2019);
    ChargenServer server(&loop, listenAddr, true);
    server.start();
    loop.loop();
    return 0;
}