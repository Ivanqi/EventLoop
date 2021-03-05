#include "EventLoop.h"
#include "TcpServer.h"

#include <stdio.h>
#include <unistd.h>

const int kBufSize = 64 * 1024
const char* g_file = NULL;

void onHighWaterMark(const TcpConnectionPtr& conn) {
    std::string state = (conn->connected() ? "UP" : "DOWN");
    printf("DiscardServer - %s -> %s is %s\n", conn->peerAddress().toIpPort().c_str(), 
    conn->localAddress().toIpPort().c_str(), state.c_str());

    if (conn->connected()) {
        printf("FileServer - Sending file %s to %s\n", g_file, conn->peerAddress().toIpPort().c_str());
        conn->setHighWaterMarkCallback(onHighWaterMark, kBufSize + 1);

        FILE *fp = ::fopen(g_file, "rb");
        if (fp) {
            conn->setContext(fp);
            char buf[kBufSize];
            size_t nread = ::fread(buf, 1, sizeof(buf), fp);
            conn->send(buf, static_cast<int>(nread));
        } else {
            conn->shutdown();
            printf("FileServer - no such file\n");
        }
    } else {
        if (!conn->getContext().empty()) {
            FILE *fp = boost::any_cast<FILE*>(conn->getContext());
            if (fp) {
                ::fclose(fp);
            }
        }
    }
}

void onWriteComplete(const TcpConnectionPtr& conn)
{
    FILE *fp = boost::any_cast<FILE*>(conn->getContext());
    char buf[kBufSize];
    size_t nread = ::fread(buf, 1, sizeof(buf), fp);
    if (nread > 0) {
        conn->send(buf, static_cast<int>(nread));
    } else {
        ::fclose(fp);
        fp = NULL;
        conn->setContext(fp);
        conn->shutdown();
        printf("FileServer - done\n");
    }
}

int main() {

    printf("pid = %d\n", getpid());
    if (argc > 1) {
        g_file = argv[1];

        EventLoop loop;
        InetAddress listenAddr(2021);
        TcpServer server(&loop, listenAddr, "FileServer");
        server.setConnectionCallback(onConnection);
        server.setWriteCompleteCallback(onWriteComplete);
        server.start();
        loop.loop();
    } else {
        fprintf(stderr, "Usage: %s file_for_downloading\n", argv[0]);
    }
}