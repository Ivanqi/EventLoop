#include "TcpServer.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "SocketsOps.h"

#include <stdio.h>  // snprintf

TcpServer::TcpServer(EventLoop *loop, const InetAddress& listenAddr, const string& nameArg, Option option)
    :loop_(loop), ipPort(listenAddr.toIpPort()), name_(nameArg),
    acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
    connectionCallback_(defaultConnectionCallback), messageCallback_(defaultMessageCallback),
    nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
    loop_->assertInLoopThread();

    for (auto& item: connections_) {
        TcpConnectionPtr conn(item.second);
        item.sencond.reset();

        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn)
        );
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    assert(0 <= numThreads);
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if (started_.getAndSet(1) == 0) {
        threadPool_->start(threadInitCallback_);

        assert(!acceptor_->listenning());

        loop->runInLoop(
            std::bind(&Acceptor::listen, get_pointer(acceptor_));
        );
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    loop_->assertInLoopThread();
    EventLoop *ioLoop = threadPool_->getNextLoop();

    char buf[64];
    snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;

    string connName = name_ + buf;

    InetAddress localAddr(sockets::getLocalAddr(sockfd));

    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));

    connections_[connName] = conn;

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, _1)   // 线程不安全
    );

    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    size_t n = connections_.erase(conn->name());
    (void)n;
    assert(n == 1);
    EventLoop* ioLoop = conn->getLoop();

     ioLoop->queueInLoop(
      std::bind(&TcpConnection::connectDestroyed, conn));
}