#include "EventLoop.h"
#include "TcpClient.h"

#include <stdio.h>

TcpClient *g_client;

void timeout() {

    printf("timeout\n");
    g_client->stop();
}

int main(int argc, char *argv[]) {

    EventLoop loop;
    InetAddress serverAddr("127.0.0.1", 2);
    TcpClient client(&loop, serverAddr, "TcpClient");

    g_client = &client;

    loop.runAfter(0.0, timeout);
    loop.runAfter(1.0, std::bind(&EventLoop::quit, &loop));
    client.connect();

    CurrentThread::sleepUsec(100 * 1000);

    loop.loop();

    return 0;
}