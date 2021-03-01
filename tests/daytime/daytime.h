#ifndef EVENT_TEST_DAYTIME_H
#define EVENT_TEST_DAYTIME_H

#include "TcpServer.h"

class DaytimeServer
{
    private:
        void onConnection(const TcpConnectionPtr& conn);

        void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time);

        TcpServer server_;
    
    public:
        DaytimeServer(EventLoop* loop, const InetAddress& listenAddr);

        void start();
};

#endif