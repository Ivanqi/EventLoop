#include "EventLoop.h"
#include "TcpServer.h"
#include <list>
#include <stdio.h>
#include <unistd.h>

class EchoServer
{
    private:
        typedef std::weak_ptr<TcpConnection> WeakTcpConnectionPtr;
        typedef std::list<WeakTcpConnectionPtr> WeakConnectionList;

        TcpServer server_;
        int idleSeconds_;
        WeakConnectionList connectionList_;

        struct Node
        {
            Timestamp lastReceiveTime;
            WeakConnectionList::iterator position;
        };

    public:
        EchoServer(EventLoop *loop, const InetAddress& listenAddr, int idleSecond);

        void start()
        {
            server_.start();
        }
    
    private:
        void onConnection(const TcpConnectionPtr& conn);

        void onMessage(const TcpConnectionPtr& conn, Buffer *buf, Timestamp time);

        void onTimer();

        void dumpConnectionList() const;
};

EchoServer::EchoServer(EventLoop *loop, const InetAddress& listenAddr, int idleSeconds)
    :server_(loop, listenAddr, "EchoServer"), idleSeconds_(idleSeconds)
{
    server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, _1));

    server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, _1, _2, _3));

    loop->runEvery(1.0, std::bind(&EchoServer::onTimer, this));

    dumpConnectionList();
}

void EchoServer::onConnection(const TcpConnectionPtr& conn)
{
    string state = conn->connected() ? "UP" : "DOWN";
    printf("EchoServer - %s -> %s is %s\n", conn->peerAddress().toIpPort().c_str(), conn->localAddress().toIpPort().c_str(), state.c_str());

    if (conn->connected()) {
        Node node;
        node.lastReceiveTime = Timestamp::now();
        connectionList_.push_back(conn);
        node.position = --connectionList_.end();
        conn->setContext(node);
    } else {
        assert(!conn->getContext().empty());
        const Node& node = boost::any_cast<const Node&>(conn->getContext());
        connectionList_.erase(node.position);
    }

    dumpConnectionList();
}

void EchoServer::onMessage(const TcpConnectionPtr& conn, Buffer *buf, Timestamp time)
{
    string msg(buf->retrieveAllAsString());
    printf("%s echo %d bytes at %s\n", conn->name().c_str(), (int)msg.size(), time.toString().c_str());
    conn->send(msg);

    assert(!conn->getContext().empty());
    Node *node = boost::any_cast<Node>(conn->getMutableContext());
    node->lastReceiveTime = time;
    connectionList_.splice(connectionList_.end(),connectionList_, node->position);
    assert(node->position == --connectionList_.end());

    dumpConnectionList();
}

void EchoServer::onTimer()
{
    dumpConnectionList();
    Timestamp now = Timestamp::now();

    for (WeakConnectionList::iterator it = connectionList_.begin(); it != connectionList_.end();) {
        TcpConnectionPtr conn = it->lock();
        if (conn) {
            Node *n = boost::any_cast<Node>(conn->getMutableContext());
            double age = timeDifference(now, n->lastReceiveTime);
            if (age > idleSeconds_) {
                if (conn->connected()) {
                    conn->shutdown();
                    printf("shutting down %s\n", conn->name().c_str());
                    conn->forceCloseWithDelay(3.5); // > round trip of the whole Internet.
                }
            } else if (age < 0){
                printf("Time jump\n");
                n->lastReceiveTime = now;
            } else {
                break;
            }
            ++it;
        } else {
           printf("Expired\n");
           it = connectionList_.erase(it);
        }
    }
}

void EchoServer::dumpConnectionList() const
{
    printf("size = %d\n", (int)connectionList_.size());

    for (WeakConnectionList::const_iterator it = connectionList_.begin(); it != connectionList_.end(); ++it) {
        TcpConnectionPtr conn = it->lock();
        if (conn) {
            printf("conn %p\n", get_pointer(conn));
            const Node& n = boost::any_cast<const Node&>(conn->getContext());
            printf("    time %s\n", n.lastReceiveTime.toString().c_str());
        } else {
            printf("expired\n");
        }
    }
}


int main(int argc, char* argv[]) {

    EventLoop loop;
    InetAddress listendAddr(2007);
    int idleSeconds = 10;

    if (argc > 1) {
        idleSeconds = atoi(argv[1]);
    }

    printf("pid = %d, idle seconds %d\n", getpid(), idleSeconds);
    EchoServer server(&loop, listendAddr, idleSeconds);

    server.start();
    loop.loop();

    return 0;
}