#ifndef EVENT_TEST_CHARGEN_H
#define EVENT_TEST_CHARGEN_H

#include "TcpServer.h"

class ChargenServer
{
    private:
        TcpServer server_;
        string message_;
        int64_t transferred_;
        Timestamp startTime_;

    public:
        ChargenServer(EventLoop* loop, const InetAddress& listenAddr, bool print = false);

        void start();
    
    private:
        void onConnection(const TcpConnectionPtr& conn);

        void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time);

        void onWriteComplete(const TcpConnectionPtr& conn);

        void printThroughput();
};

#endif