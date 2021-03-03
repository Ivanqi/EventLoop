#include "EventLoop.h"
#include "InetAddress.h"
#include "TcpClient.h"

#include <utility>
#include <stdio.h>
#include <unistd.h>

class ChargenClient
{
    private:
        EventLoop* loop_;
        TcpClient client_;

    public:
        ChargenClient(EventLoop* loop, const InetAddress& listenAddr)
            :loop_(loop), client_(loop, listenAddr, "ChargenClient")
        {
            client_.setConnectionCallback(std::bind(&ChargenClient::onConnection, this, _1));

            client_.setMessageCallback(std::bind(&ChargenClient::onMessage, this, _1, _2, _3));
        }

        void connect()
        {
            client_.connect();
        }
    
    private:
        void onConnection(const TcpConnectionPtr& conn)
        {
            std::string state = (conn->connected() ? "UP" : "DOWN");
            printf("ChargenServer - %s -> %s is %s\n", conn->peerAddress().toIpPort().c_str(), 
            conn->localAddress().toIpPort().c_str(), state.c_str());

            if (!conn->connected()) {
                loop_->quit();
            }
        }

        void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime)
        {
            buf->retrieveAll();
        }
};

int main(int argc, char* argv[]) {

    printf("pid = %d\n", getpid());

    if (argc > 1) {
        EventLoop loop;
        InetAddress serverAddr(argv[1], 2019);

        ChargenClient chargenClient(&loop, serverAddr);
        chargenClient.connect();
        loop.loop();
    } else {
        printf("Usage: %s host_ip\n", argv[0]);
    }

    return 0;
}