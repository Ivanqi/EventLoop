#ifndef EVENT_TEST_MAXCONNECTION_ECHO_H
#define EVENT_TEST_MAXCONNECTION_ECHO_H

#include "TcpServer.h"

class EchoServer
{
    private:
        TcpServer server_;

        int numConnected_;

        const int kMaxConnections_;

    public:
        EchoServer(EventLoop *loop, const InetAddress& listenAddr, int maxConnections);

        void start();
    
    private:
        void onConnection(const TcpConnectionPtr& conn);

        void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time);
};

#endif