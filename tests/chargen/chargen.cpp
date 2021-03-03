#include "chargen.h"
#include "EventLoop.h"

#include <stdio.h>

ChargenServer::ChargenServer(EventLoop* loop, const InetAddress& listenAddr, bool print)
    :server_(loop, listenAddr, "ChargenServer"), 
    transferred_(0), startTime_(Timestamp::now())
{

    server_.setConnectionCallback(std::bind(&ChargenServer::onConnection, this, _1));

    server_.setMessageCallback(std::bind(&ChargenServer::onMessage, this, _1, _2, _3));

    server_.setWriteCompleteCallback(std::bind(&ChargenServer::onWriteComplete, this, _1));

    if (print) {
        loop->runEvery(3.0, std::bind(&ChargenServer::printThroughput, this));
    }

    string line;
    for (int i = 33; i < 127; ++i) {
        line.push_back(char(i));
    }
    line += line;

    for (size_t i = 0; i < 127 - 33; ++i) {
        message_ += line.substr(i, 72) + '\n';
    }
}

void ChargenServer::start()
{
    server_.start();
}

void ChargenServer::onConnection(const TcpConnectionPtr& conn)
{
    std::string state = (conn->connected() ? "UP" : "DOWN");
    printf("ChargenServer - %s -> %s is %s\n", conn->peerAddress().toIpPort().c_str(), 
    conn->localAddress().toIpPort().c_str(), state.c_str());

    if (conn->connected()) {
        conn->setTcpNoDelay(true);
        conn->send(message_);
    }
}

void ChargenServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
{
    std::string msg(buf->retrieveAllAsString());
    int msgSize = msg.size();
    printf("%s discards %d bytes received at %s\n", conn->name().c_str(), msgSize, time.toString().c_str());
}

void ChargenServer::onWriteComplete(const TcpConnectionPtr& conn)
{
    transferred_ += message_.size();
    conn->send(message_);
}

void ChargenServer::printThroughput()
{
    Timestamp endTime = Timestamp::now();
    double time = timeDifference(endTime, startTime_);
    printf("%4.3f MiB/s\n", static_cast<double>(transferred_) / time / 1024 / 1024);
    transferred_ = 0;
    startTime_ = endTime;
}