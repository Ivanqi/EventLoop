#include "EventLoop.h"
#include "TcpServer.h"

#include <stdio.h>
#include <unistd.h>

void onHighWaterMark(const TcpConnectionPtr& conn, size_t len) {
    printf("HighWaterMark: %d\n", (int)len);
}

const int kBufSize = 64 * 1024;
const char* g_file = NULL;
typedef std::shared_ptr<FILE> FilePtr;

void onConnection(const TcpConnectionPtr& conn) {
    std::string state = (conn->connected() ? "UP" : "DOWN");
    printf("DiscardServer - %s -> %s is %s\n", conn->peerAddress().toIpPort().c_str(), 
    conn->localAddress().toIpPort().c_str(), state.c_str());

    if (conn->connected()) {
        printf("FileServer - Sending file %s to %s\n", g_file, conn->peerAddress().toIpPort().c_str());
        conn->setHighWaterMarkCallback(onHighWaterMark, kBufSize + 1);
        
        FILE *fp = ::fopen(g_file, "rb");
        if (fp) {
            FilePtr ctx(fp, ::fclose);
            conn->setContext(ctx);
            char buf[kBufSize];
            size_t nread = ::fread(buf, 1, sizeof(buf), fp);
            conn->send(buf, static_cast<int>(nread));
        } else {
            conn->shutdown();
            printf("FileServer - no such file\n");
        }
    }
}

void onWriteComplete(const TcpConnectionPtr& conn) {
    const FilePtr& fp = boost::any_cast<const FilePtr&>(conn->getContext());
    char buf[kBufSize];
    size_t nread = ::fread(buf, 1, sizeof buf, get_pointer(fp));

    if (nread > 0) {
        conn->send(buf, static_cast<int>(nread));
    } else {
        conn->shutdown();
        printf("FileServer - done\n");
    }
}

int main(int argc, char* argv[]) {

    printf("pid = %d\n", getpid());
    if (argc > 1) {
        g_file = argv[1];

        EventLoop loop;
        InetAddress listenAddr(2021);
        TcpServer server(&loop, listenAddr, "FIleServer");

        server.setConnectionCallback(onConnection);
        server.setWriteCompleteCallback(onWriteComplete);
        server.start();
        loop.loop();
    }
    return 0;
}