#include "EventLoop.h"
#include "TcpServer.h"

#include <stdio.h>
#include <unistd.h>

const char* g_file = NULL;

string readFile(const char* filename) 
{
    string content;
    FILE *fp = ::fopen(filename, "rb");

    if (fp) {
        const int kBuffSize = 1024 * 1024;
        char iobuf[kBuffSize];
        ::setbuffer(fp, iobuf, sizeof(iobuf));

        char buf[kBuffSize];
        size_t nread = 0;
        
        while ((nread = ::fread(buf, 1, sizeof(buf), fp)) > 0) {
            content.append(buf, nread);
        }
        ::fclose(fp);
    }
    return content;
}

void onHighWaterMark(const TcpConnectionPtr& conn, size_t len) {
    printf("HighWaterMark %d\n", (int)len);
}

void onConnection(const TcpConnectionPtr& conn)
{
    std::string state = (conn->connected() ? "UP" : "DOWN");
    printf("DiscardServer - %s -> %s is %s\n", conn->peerAddress().toIpPort().c_str(), 
    conn->localAddress().toIpPort().c_str(), state.c_str());

    if (conn->connected()) {
        printf("FileServer - Sending file %s to %s\n", g_file, conn->peerAddress().toIpPort().c_str());
        conn->setHighWaterMarkCallback(onHighWaterMark, 64 * 1024);
        string fileContent = readFile(g_file);
        conn->send(fileContent);
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
        TcpServer server(&loop, listenAddr, "FileServer");
        server.setConnectionCallback(onConnection);
        server.start();
        loop.loop();
    } else {
        fprintf(stderr, "Usage: %s file_for_downloading\n", argv[0]);
    }

    return 0;    
}