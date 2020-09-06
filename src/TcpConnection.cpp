#include "TcpConnection.h"
#include "WeakCallback.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "SocketsOps.h"

#include <errno.h>

void defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    string connectState = conn->connected() ? "UP" : "DOWN";
    printf("%s -> %s is %s", conn->localAddress().toIpPort(), conn->peerAddress().toIpPort(), connectState);
}

void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer *buf, Timestamp)
{
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop *loop, const string& nameArg, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr)
    :loop_(loop), name_(nameArg), state_(kConnecting), reading_(true), 
    socket_(new Socket(sockfd)), channel_(new Channel(loop, sockfd)), 
    localAddr_(localAddr), peerAddr_(peerAddr), highWaterMark_(64 * 1024 * 1024)
{
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));

    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));

    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));

    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    assert(state_ == kDisconnected);
}

string TcpConnection::getTcpInfoString() const
{
    char buf[1024];
    buf[0] = '\0';
    socket_->getTcpInfoString(buf, sizeof(buf));
    return buf;
}

void TcpConnection::send(const void *data, int len)
{
    send(StringPiece(static_cast<const char*>(data), len));
}

void TcpConnection::send(const StringPiece& message)
{
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(message);
        } else {
            void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                std::bind(fp, this, message.as_string())
            );
        }
    }
}

void TcpConnection::send(Buffer *buf)
{
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        } else {
            void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(std::bind(fp, this, buf->retrieveAllAsString()));
        }
    }
}

void TcpConnection::sendInLoop(const StringPiece& message)
{
    sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if (state_ == kDisconnected) {
        return ;
    }

    // 如果输出队列中没有任何内容，请尝试直接写入
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = sockets::write(channel_->fd(), data, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_) {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
        }
    }

    assert(remaining <= len);
    if (!faultError && remaining > 0) {
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_) {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }

        outputBuffer_.append(static_cast<const char *>(data) + nwrote, remaining);

        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdown()
{
    // 使用比较和交换
    if (state_ == kConnected) {
        setState(kDisconnected);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if (!channel_->isWriting()) {
        socket_->shutdownWrite();
    }
}

void TcpConnection::forceClose()
{
    // 使用比较和交换
    if (state_ == kConnected || state_ == kDisconnected) {
        setState(kDisconnecting);
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}