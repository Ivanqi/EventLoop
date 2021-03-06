#ifndef EVENT_TEST_ECHO_H
#define EVENT_TEST_ECHO_H

#include "TcpServer.h"

class EchoServer
{
    private:
        TcpServer server_;

    public:
        EchoServer(EventLoop *loop, const InetAddress& listenAddr);

        void start();

    private:
        void onConnection(const TcpConnectionPtr& conn);

        void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time);
};

#endif