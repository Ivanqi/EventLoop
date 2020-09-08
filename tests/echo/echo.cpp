#include "echo.h"
#include <stdio.h>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

EchoServer::EchoServer(EventLoop *loop, InetAddress& listenAddr)
    :server_(loop, listenAddr, "EchoServer")
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
    string state = conn->connected() ? "UP" : "DOWN";
    printf("EchoServer - %s -> %s is %s\n", conn->peerAddress().toIpPort(), conn->localAddress().toIpPort(), state);
}

void EchoServer::onMessage(TcpConnectionPtr& conn, Buffer *buf, Timestamp time)
{
    string msg(buf->retrieveAllAsString());
    printf("%s echo %d bytes, data received at %s\n", conn->name().c_str(), (int)msg.size(), time.toString().c_str());
    conn->send(msg);
}