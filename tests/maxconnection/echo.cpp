#include "echo.h"

EchoServer::EchoServer(EventLoop *loop, const InetAddress& listenAddr, int maxConnections)
    :server_(loop, listenAddr, "EchoServer"), numConnected_(0), kMaxConnections_(maxConnections)
{
    server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, _1));

    server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, _1, _2, _3));
}

void EchoServer::start()
{
    server_.start();
}
    
void EchoServer::onConnection(const TcpConnectionPtr& conn)
{
    std::string state = (conn->connected() ? "UP" : "DOWN");
    printf("DiscardServer - %s -> %s is %s\n", conn->peerAddress().toIpPort().c_str(), 
    conn->localAddress().toIpPort().c_str(), state.c_str());

    if (conn->connected()) {
        ++numConnected_;
        if (numConnected_ > kMaxConnections_) {
            conn->shutdown();
            conn->forceCloseWithDelay(3.0);
        }
    } else {
        --numConnected_;
    }

    printf("numConnected = %d\n", numConnected_);
}

void EchoServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
{
    string msg(buf->retrieveAllAsString());
    printf("%s echo %d bytes at %s\n", conn->name().c_str(), (int)msg.size(), time.toString().c_str());
}