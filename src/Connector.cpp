#include "Connector.h"
#include "Channel.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include <errno.h>
#include <stdio.h>

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop *loop, const InetAddress& serverAddr)
    : loop_(loop), serverAddr_(serverAddr), connect_(false), state_(kDisconnected), retryDelayMs_(kInitRetryDelayMs)
{

}

Connector::~Connector()
{
    assert(!channel_);
}

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_) {
        connect();
    } else {
        printf("do not connect\n");
    }
}

void Connector::stop()
{
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));    // FIXME: unsafe
}

void Connector::stopInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == kConnecting) {
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

void Connector::connect()
{
    int sockfd = sockets::createNonblockingOrDie(serverAddr_.family());
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddr());
    int savedErrno = (ret == 0) ? 0 : errno;

    switch (savedErrno) {
        case 0:
        case EINPROGRESS:
        case EINTR:
        case EISCONN:
            connecting(sockfd);
            break;
        
        case EAGAIN:
        case EADDRINUSE:
        case EADDRNOTAVAIL:
        case ECONNREFUSED:
        case ENETUNREACH:
            retry(sockfd);
            break;
        
        case EACCES:
        case EPERM:
        case EAFNOSUPPORT:
        case EALREADY:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            printf("connect error in Connector::startInLoop %d\n", savedErrno);
            sockets::close(sockfd);
            break;
        
        default:
            printf("Unexpected error in Connector::startInLoop %d\n", savedErrno);
            sockets::close(sockfd);
            break;
    }
}

void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::connecting(int sockfd)
{
    setState(kConnecting);
    assert(!channel_);

    channel_.reset(new Channel(loop_, sockfd));

    channel_->setWriteCallback(std::bind(&Connector::handleWrite, this)); // FIXME: unsafe

    channel_->setErrorCallback(std::bind(&Connector::handleError, this)); // FIXME: unsafe

    channel_->enableWriting();
}

int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();

    int sockfd = channel_->fd();
    // Can't reset channel_ here, because we are inside Channel::handleEvent
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this));  // FIXME: unsafe
    return sockfd;
}

void Connector::resetChannel()
{
    channel_.reset();
}

void Connector::handleWrite()
{
    printf("Connector::handleWrite %d\n", state_);

    if (state_ == kConnecting) {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);

        if (err) {
            printf("Connector::handleWrite - SO_ERROR = %d\n", err);
            retry(sockfd);

        } else if (sockets::isSelfConnect(sockfd)) {
            printf("Connector::handleWrite - Self connect\n");
            retry(sockfd);

        } else {
            setState(kConnected);
            if (connect_) {
                newConnectionCallback_(sockfd);
            } else {
                sockets::close(sockfd);
            }
        }
    } else {
        // what heappend?
        assert(state_ == kDisconnected);
    }
}

void Connector::handleError()
{
    printf("Connector::handleError state=%d\n", state_);
    if (state_ == kConnecting) {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        printf("SO_ERROR = %d\n", err);
        retry(sockfd);
    }
}

void Connector::retry(int sockfd)
{
    sockets::close(sockfd);
    setState(kDisconnected);

    if (connect_) {
        printf("Connector::retry -- Retry connecting to %s in %d milliseconds.\n", serverAddr_.toIpPort().c_str(), retryDelayMs_);

        loop_->runAfter(retryDelayMs_ / 1000.0, std::bind(&Connector::startInLoop, shared_from_this()));

        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    } else {
        printf("do not connect\n");
    }
}