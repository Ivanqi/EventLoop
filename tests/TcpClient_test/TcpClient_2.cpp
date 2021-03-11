#include "Thread.h"
#include "EventLoop.h"
#include "TcpClient.h"

void threadFunc(EventLoop *loop)
{
    InetAddress serverAddr("127.0.0.1", 1234);
    TcpClient client(loop, serverAddr, "TcpClient");
    client.connect();

    CurrentThread::sleepUsec(1000 * 1000);
}

int main() {

    EventLoop loop;
    loop.runAfter(3.0, std::bind(&EventLoop::quit, &loop));
    Thread thr(std::bind(threadFunc, &loop));
    thr.start();
    loop.loop();
    return 0;
}