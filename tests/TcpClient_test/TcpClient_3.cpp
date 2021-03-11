#include "TcpClient.h"
#include "Thread.h"
#include "EventLoop.h"
#include "InetAddress.h"

#include <utility>
#include <stdio.h>
#include <unistd.h>

int numThreads = 0;
class EchoClient;
std::vector<std::unique_ptr<EchoClient>> clients;
int current = 0;

class EchoClient
{
    private:
        EventLoop *loop_;
        TcpClient client_;
    public:
        EchoClient(EventLoop *loop, const InetAddress& listenAddr, const string& id)
            :loop_(loop), client_(loop, listenAddr, "EchoClient" + id)
        {
            client_.setConnectionCallback(std::bind(&EchoClient::onConnection, this, _1));

            client_.setMessageCallback(std::bind(&EchoClient::onMessage, this, _1, _2, _3));
        }

        void connect()
        {
            client_.connect();
        }
    
    private:
        void onConnection(const TcpConnectionPtr& conn)
        {
            string state = conn->connected() ? "UP" : "DOWN";
            printf("EchoClient - %s -> %s is %s\n", conn->peerAddress().toIpPort().c_str(), conn->localAddress().toIpPort().c_str(), state.c_str());

            if (conn->connected()) {
                ++current;
                if (implicit_cast<size_t>(current) < clients.size()) {
                    clients[current]->connect();
                }
                printf("*** connected\n");
            }
            conn->send("wolrd\n");
        }

        void onMessage(const TcpConnectionPtr& conn, Buffer *buf, Timestamp time)
        {
            string msg(buf->retrieveAllAsString());
            printf("%s echo %d bytes, data received at %s\n", conn->name().c_str(), (int)msg.size(), time.toString().c_str());
            if (msg == "quit\n") {
                conn->send(msg);
                conn->shutdown();            
            } else if (msg == "shutdown\n") {
                loop_->quit();
            } else {
                conn->send(msg);
            }
        }
};

int main(int argc, char* argv[]) {

    printf("pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    if (argc > 1) {
        EventLoop loop;
        bool ipv6 = argc > 3;
        InetAddress serverAddr(argv[1], 2000, ipv6);

        int n = 1;
        if (argc > 2) {
            n = atoi(argv[2]);
        }

        clients.reserve(n);
        for (int i = 0; i < n; ++i) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", i + 1);
            clients.emplace_back(new EchoClient(&loop, serverAddr, buf));
        }

        clients[current]->connect();
        loop.loop();
    }

    return 0;
}