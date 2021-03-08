#include "TcpServer.h"
#include "Atomic.h"
#include "Thread.h"
#include "EventLoop.h"
#include "InetAddress.h"

#include <utility>
#include <stdio.h>
#include <unistd.h>

void onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected()) {
        conn->setTcpNoDelay(true);
    }
}

void onMessage(const TcpConnectionPtr& conn, Buffer *buf, Timestamp)
{
    conn->send(buf);
}

int main(int argc, char* argv[]) {
    
    if (argc < 4) {
        fprintf(stderr, "Usage: server <address> <port> <threads>\n");
    } else {
        printf("pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
        
        const char* ip = argv[1];
        uint16_t port = static_cast<uint16_t>(atoi(argv[2]));

        InetAddress listenAddr(ip, port);
        int threadCount = atoi(argv[3]);

        EventLoop loop;

        TcpServer server(&loop, listenAddr, "PingPong");

        server.setConnectionCallback(onConnection);
        server.setMessageCallback(onMessage);

        if (threadCount > 1) {
            server.setThreadNum(threadCount);
        }

        server.start();

        loop.loop();
    }
    return 0;
}