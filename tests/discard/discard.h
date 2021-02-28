#ifndef EVENT_TEST_DISCARD_H
#define EVENT_TEST_DISCARD_H

#include "TcpServer.h"
#include <stdio.h>

class DiscardServer
{
    private:
        void onConnection(const TcpConnectionPtr& conn);

        void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time);

        TcpServer server_;
    
    public:
        DiscardServer(EventLoop *loop, const InetAddress& listenAddr);

        void start();
};
#endif