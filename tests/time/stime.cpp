#include "stime.h"
#include "EventLoop.h"
#include <stdio.h>

TimeServer::TimeServer(EventLoop* loop, const InetAddress& listenAddr)
    :server_(loop, listenAddr, "TimeServer")
{
    server_.setConnectionCallback(std::bind(&TimeServer::onConnection, this, _1));

    server_.setMessageCallback(std::bind(&TimeServer::onMessage, this, _1, _2, _3));
}

void TimeServer::start()
{
    server_.start();
}

void TimeServer::onConnection(const TcpConnectionPtr& conn)
{
    std::string state = (conn->connected() ? "UP" : "DOWN");
    printf("DiscardServer - %s -> %s is %s\n", conn->peerAddress().toIpPort().c_str(), 
    conn->localAddress().toIpPort().c_str(), state.c_str());

    if (conn->connected()) {
        time_t now = ::time(NULL);
        int32_t be32 = hostToNetwork32(static_cast<int32_t>(now));
        conn->send(&be32, sizeof(be32));
        conn->shutdown();
    }
}

void TimeServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
{
    std::string msg(buf->retrieveAllAsString());
    int msgSize = msg.size();
    printf("%s daytime %d bytes received at %s\n", conn->name().c_str(), msgSize, time.toString().c_str());
}