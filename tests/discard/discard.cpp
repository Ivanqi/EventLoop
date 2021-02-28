#include "discard.h"

DiscardServer::DiscardServer(EventLoop *loop, const InetAddress& listenAddr)
    :server_(loop, listenAddr, "DiscardServer")
{
    server_.setConnectionCallback(std::bind(&DiscardServer::onConnection, this, _1));

    server_.setMessageCallback(std::bind(&DiscardServer::onMessage, this, _1, _2, _3));
}

void DiscardServer::start()
{
    server_.start();
}

void DiscardServer::onConnection(const TcpConnectionPtr& conn)
{
    std::string state = (conn->connected() ? "UP" : "DOWN");
    printf("DiscardServer - %s -> %s is %s\n", conn->peerAddress().toIpPort().c_str(), 
    conn->localAddress().toIpPort().c_str(), state.c_str());
}

void DiscardServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
{
    std::string msg(buf->retrieveAllAsString());
    int msgSize = msg.size();
    printf("%s discards %d bytes received at %s\n", conn->name().c_str(), msgSize, time.toString().c_str());
}