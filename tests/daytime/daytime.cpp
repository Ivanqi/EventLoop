#include "daytime.h"
#include "EventLoop.h"
#include <stdio.h>

DaytimeServer::DaytimeServer(EventLoop* loop, const InetAddress& listenAddr)
    :server_(loop, listenAddr, "DaytimeServer")
{
    server_.setConnectionCallback(std::bind(&DaytimeServer::onConnection, this, _1));

    server_.setMessageCallback(std::bind(&DaytimeServer::onMessage, this, _1, _2, _3));
}

void DaytimeServer::start()
{
    server_.start();
}

void DaytimeServer::onConnection(const TcpConnectionPtr& conn)
{
    std::string state = (conn->connected() ? "UP" : "DOWN");
    printf("DiscardServer - %s -> %s is %s\n", conn->peerAddress().toIpPort().c_str(), 
    conn->localAddress().toIpPort().c_str(), state.c_str());

    if (conn->connected()) {
        conn->send(Timestamp::now().toFormattedString() + "\n");
        conn->shutdown();
    }
}

void DaytimeServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
{
    std::string msg(buf->retrieveAllAsString());
    int msgSize = msg.size();
    printf("%s daytime %d bytes received at %s\n", conn->name().c_str(), msgSize, time.toString().c_str());
}