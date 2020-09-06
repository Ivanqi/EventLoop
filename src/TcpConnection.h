#ifndef EVENT_TCPCONNECTION
#define EVENT_TCPCONNECTION

#include "StringPiece.h"
#include "Types.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "InetAddress.h"

#include <memory>
#include <boost/any.hpp>


struct tcp_info;

class Channel;
class EventLoop;
class Socket;

/**
 * TCP连接，用于客户端和服务器
 */
class TcpConnection
{
    private:
        enum StateE {kDisconnected, kConnecting, kConnected, kDisconnecting};
        EventLoop *loop_;
        const string name_;
        StateE state_;  // 使用原子变量
        bool reading_;

        std::unique_ptr<Socket> socket_;
        std::unique_ptr<Channel> channel_;

        const InetAddress localAddr_;   // 本地地址
        const InetAddress peerAddr_;    // 对端地址

        // 回调函数
        ConnectionCallback connectionCallback_;
        MessageCallback messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        HighWaterMarkCallback highWaterMarkCallback_;
        CloseCallback closeCallback;
        size_t highWaterMark_;
        
        Buffer inputBuffer_;
        Buffer outputBuffer_;
        boost::any context_;

    public:
        TcpConnection(EventLoop *loop, const string& name, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);

        ~TcpConnection();

        EventLoop* getLoop() const
        {
            return loop_;
        }

        const string& name() const
        {
            return name_;
        }

        const InetAddress& localAddress() const
        {
            return localAddr_;
        }

        const InetAddress& peerAddress() const
        {
            return peerAddr_;
        }

        bool connected() const
        {
            return state_ == kConnected;
        }

        bool disconnected() const
        {
            return state_ == kDisconnected;
        }

        // 成功返回true
        bool getTcpInfo(struct tcp_info*) const;

        string getTcpInfoString() const;

        void send(const void* message, int len);

        void send(const StringPiece& message);

        void send(Buffer *message); // 这个会交换数据

        void shutdown();    // 不是线程安全的，不能同时调用

        void forceClose();

        void forceCloseWithDelay(double seconds);

        void setTcpNoDelay(bool on);

        void startRead();

        void stopRead();

        // 不是线程安全的，可能与start/stopReadInLoop竞争
        bool isReading() const
        {
            return reading_;    
        }

        void setContext(const boost::any& context)
        {
            context_ = context;
        }

        boost::any* getMutableContext()
        {
            return &context_;
        }

        void setConnectionCallback(const ConnectionCallback& cb)
        {
            connectionCallback_ = cb;
        }

        void setMessageCallback(const MessageCallback& cb)
        {
            messageCallback_ = cb;
        }

        void setWriteCompleteCallback(const WriteCompleteCallback& cb)
        {
            writeCompleteCallback_ = cb;
        }

        void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
        {
            highWaterMarkCallback_ = cb;
            highWaterMark_ = highWaterMark;
        }

        Buffer* inputBuffer()
        {
            return &inputBuffer_;
        }

        Buffer* outputBuffer()
        {
            return &outputBuffer_;
        }

        // 仅供内部使用
        void setCloseCallback(const CloseCallback& cb)
        {
            closeCallback = cb;
        }

        // 当TcpServer接受新连接时调用
        void connectEstablished();  // 应该只调用一次

        // 当TcpServer将我从其映射中删除时调用
        void connectDestroyed();    // 应该只调用一次
    
    private:
        void handleRead(Timestamp receiveTime);

        void handleWrite();

        void handleClose();

        void handleError();

        void sendInLoop(const StringPiece& message);

        void sendInLoop(const void* message, size_t len);

        void shutdownInLoop();

        void forceCloseInLoop();

        void setState(StateE s)
        {
            state_ = s;
        }

        const char* stateToString() const;

        void startReadInLoop();

        void stopReadInLoop();
};

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

#endif