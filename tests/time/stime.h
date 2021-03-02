#ifndef EVENT_TEST_TIME_H
#define EVENT_TEST_TIME_H

#include "TcpServer.h"

class TimeServer
{
    private:
        void onConnection(const TcpConnectionPtr& conn);

        void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time);

        TcpServer server_;
    
    public:
        TimeServer(EventLoop* loop, const InetAddress& listenAddr);

        void start();
};

#endif