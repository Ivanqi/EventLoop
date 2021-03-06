#ifndef EVENT_TCPCONNECTION
#define EVENT_TCPCONNECTION

#include "StringPiece.h"
#include "Types.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "InetAddress.h"

#include <memory>   // shared_from_this
#include <boost/any.hpp>
#include <boost/noncopyable.hpp>


struct tcp_info;

class Channel;
class EventLoop;
class Socket;

/**
 * TCP连接，用于客户端和服务器
 * TcpConnection 表示的是“一次TCP连接”，它是不可再生的，一旦连接断开，这个TcpConnection对象就没啥用来
 * 另外TcpConnection没有发起连接的功能，其构造函数的参数是已经建立好连接的sock fd(无论是TcpServer被动接受还是TcpClient主动发起)
 * 因此其初始状态是kConnecting
 */
class TcpConnection: boost::noncopyable, public std::enable_shared_from_this<TcpConnection>
{
    private:
        enum StateE {kDisconnected, kConnecting, kConnected, kDisconnecting};
        EventLoop *loop_;   // ioLoop
        const string name_; // 连接名称
        StateE state_;  // 使用原子变量，状态机
        bool reading_;  // 事件是否可读

        std::unique_ptr<Socket> socket_;    // 新进连接的fd
        std::unique_ptr<Channel> channel_;  // ioLoop 的channel

        const InetAddress localAddr_;   // 本地地址
        const InetAddress peerAddr_;    // 对端地址

        // 回调函数
        ConnectionCallback connectionCallback_; // 连接回调
        MessageCallback messageCallback_;       // 信息回调
        WriteCompleteCallback writeCompleteCallback_;   // 写入完成回调
        HighWaterMarkCallback highWaterMarkCallback_;   // 高水位回调
        CloseCallback closeCallback_;           // 关闭回调
        size_t highWaterMark_;
        
        // 使用buffer作为缓冲
        Buffer inputBuffer_;
        Buffer outputBuffer_;
        boost::any context_;    // http context

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

        const boost::any& getContext() const
        { 
            return context_; 
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

        /**
         * TcpConnection class 也新增了CloseCallback事件回调，但是这个回调是给TcpServer和TcpClient用的
         * 用于通知它们移除所持有的TcpConnectionPtr，这不是给普通用户用的，普通用户继续使用ConnectionCallback
         */
        void setCloseCallback(const CloseCallback& cb)
        {
            closeCallback_ = cb;
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