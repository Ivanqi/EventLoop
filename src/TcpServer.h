#ifndef EVENT_TCPSERVER_H
#define EVENT_TCPSERVER_H

#include "Atomic.h"
#include "Types.h"
#include "TcpConnection.h"
#include <boost/noncopyable.hpp>

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

/**
 * TCP服务器，支持单线程和线程池模型
 */
class TcpServer: boost::noncopyable
{
    public:
        typedef std::function<void(EventLoop*)> ThreadInitCallback;

        enum Option {
            kNoReusePort, kReusePort
        };

    private:
        // 不是线程安全的，而是在循环
        void newConnection(int sockfd, const InetAddress& peerAddr);

        // 线程安全
        void removeConnection(const TcpConnection& conn);

        // 不是线程安全的，而是在循环
        void removeConnctionInLoop(const TcpConnectionPtr& conn);

        typedef std::map<string, TcpConnectionPtr> ConectionMap;

        EventLoop* loop_;   // the acceptor loop

        const string inPort_;

        const string name_;

        std::unique_ptr<Acceptor> acceptor_;    // 避免暴露Acceptor

        std::shared_ptr<EventLoopThreadPool> threadPool_;

        ConnectionCallback connectionCallback_;

        MessageCallback messageCallback_;

        WriteCompleteCallback writeCompleteCallback_;
        
        ThreadInitCallback threadInitCallback_;

        AtomicInt32 started_;

        int nextConnId_;

        ConectionMap connections_;

    public:
        TcpServer(EventLoop *loop, const InetAddress& listenAddr, const string& nameArg, Option option = kNoReusePort);

        ~TcpServer();

        const string& ipPort() const
        {
            return ipPort_;
        }

        const string& name() const
        {
            return name_;
        }

        EventLoop* getLoop() const
        {
            return loop_;
        }
};

#endif