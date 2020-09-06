#ifndef EVENT_ACCEPTOR_H
#define EVENT_ACCEPTOR_H

#include <functional>

#include "Channel.h"
#include "Socket.h"

class EventLoop;
class InetAddress;

/**
 * 传入TCP连接的接受者
 */
class Acceptor
{
    public:
        typedef std::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;

    private:
        EventLoop *loop_;
        Socket acceptSocket_;
        Channel acceptChannel_;
        NewConnectionCallback newConnectionCallback_;
        bool listenning_;
        int idleFd_;
    
    public:
        Acceptor(EventLoop *loop, const InetAddress& listenAddr, bool reuseport);

        ~Acceptor();

        void setNewConnectionCallback(const NewConnectionCallback& cb)
        {
            newConnectionCallback_ = cb;
        }

        bool listenning() const
        {
            return listenning_;
        }

        void listen();
    
    private:
        void handleRead();

};

#endif