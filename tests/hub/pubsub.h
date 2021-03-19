#ifndef EVENT_TEST_PUBSUB_H
#define EVENT_TEST_PUBSUB_H

#include "TcpClient.h"
using std::string;

class PubSubClient
{
    public:
        typedef std::function<void (PubSubClient*)> ConnectionCallback;
        typedef std::function<void (const string& topic, const string& content, Timestamp)> SubscribeCallback;
    
    private:
        TcpClient client_;
        TcpConnectionPtr conn_;
        ConnectionCallback connectionCallback_;
        SubscribeCallback subscribeCallback_;
    
    public:
        PubSubClient(EventLoop *loop, const InetAddress& hubAddr, const string& name);

        void start();

        void stop();

        bool connected() const;

        void setConnectionCallback(const ConnectionCallback& cb)
        {
            connectionCallback_ = cb;
        }

        bool subscribe(const string& topic, const SubscribeCallback& cb);

        void unsubscribe(const string& topic);

        bool publish(const string& topic, const string& content);

    private:
        void onConnection(const TcpConnectionPtr& conn);

        void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime);

        bool send(const string& message);
};

#endif