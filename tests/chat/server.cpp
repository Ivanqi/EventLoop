#include "codec.h"
#include "MutexLock.h"
#include "EventLoop.h"
#include "TcpServer.h"

#include <set>
#include <stdio.h>
#include <unistd.h>

class ChatServer
{
    private:
        typedef std::set<TcpConnectionPtr> ConnectionList;
        TcpServer server_;
        LengthHeaderCodec codec_;
        ConnectionList connections_;
    
    public:
        ChatServer(EventLoop *loop, const InetAddress& listenAddr)
            :server_(loop, listenAddr, "ChatServer"), codec_(std::bind(&ChatServer::onStringMessage, this, _1, _2, _3))
        {
            server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

            server_.setMessageCallback(std::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
        }

        void start()
        {
            server_.start();
        }
    
    private:
        void onConnection(const TcpConnectionPtr& conn)
        {
            string state = conn->connected() ? "UP" : "DOWN";
            printf("EchoServer - %s -> %s is %s\n", conn->peerAddress().toIpPort().c_str(), conn->localAddress().toIpPort().c_str(), state.c_str());

            if (conn->connected()) {
                connections_.insert(conn);
            } else {
                connections_.erase(conn);
            }
        }

        void onStringMessage(const TcpConnectionPtr&, const string& message, Timestamp)
        {
            for (ConnectionList::iterator it = connections_.begin(); it != connections_.end(); ++it) {
                codec_.send(get_pointer(*it), message);
            }
        }
};

int main(int argc, char *argv[]) {

    printf("pid = %d\n", getpid());
    if (argc > 1) {
        EventLoop loop;
        uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
        InetAddress serverAddr(port);
        ChatServer server(&loop, serverAddr);
        server.start();
        loop.loop();
    }
    return 0;
}