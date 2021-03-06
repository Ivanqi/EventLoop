#ifndef EVENT_HTTPSERVER_H
#define EVENT_HTTPSERVER_H

#include "TcpServer.h"

class HttpRequest;
class HttpResponse;

class HttpServer
{
    public:
        typedef std::function<void (const HttpRequest&, HttpResponse*)> HttpCallback;

    private:
        TcpServer server_;
        HttpCallback httpCallback_;
    
    public:
        HttpServer(EventLoop* loop, const InetAddress& listenAddr, const string& name, TcpServer::Option option = TcpServer::kNoReusePort);

        EventLoop* getLoop() const
        {
            return server_.getLoop();
        }

        // 不是线程安全。调用start()之前注册回调
        void setHttpCallback(const HttpCallback& cb)
        {
            httpCallback_ = cb;
        }

        void setThreadNum(int numThreads)
        {
            server_.setThreadNum(numThreads);
        }

        void start();
    
    private:
        void onConnection(const TcpConnectionPtr& conn);

        void onMessage(const TcpConnectionPtr& conn, Buffer *buf, Timestamp receiveTime);

        void onRequest(const TcpConnectionPtr& , const HttpRequest&);
};

#endif