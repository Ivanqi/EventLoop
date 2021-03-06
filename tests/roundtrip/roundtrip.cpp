#include "EventLoop.h"
#include "TcpClient.h"
#include "TcpServer.h"

#include <stdio.h>

const size_t frameLen = 2 * sizeof(int64_t);

void serverConnectionCallback(const TcpConnectionPtr& conn) {
    std::string state = (conn->connected() ? "UP" : "DOWN");
    printf("DiscardServer - %s -> %s is %s\n", conn->peerAddress().toIpPort().c_str(), 
    conn->localAddress().toIpPort().c_str(), state.c_str());

    if (conn->connected()) {
       conn->setTcpNoDelay(true);
    } else {
    }
}

void serverMessageCallback(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime) {
    int64_t message[2];

    while (buffer->readableBytes() >= frameLen) {
        memcpy(message, buffer->peek(), frameLen);
        buffer->retrieve(frameLen);
        message[1] = receiveTime.microSecondsSinceEpoch();
        conn->send(message, sizeof(message));
    }
}

void runServer(uint16_t port) {
    EventLoop loop;
    TcpServer server(&loop, InetAddress(port), "ClockServer");
    server.setConnectionCallback(serverConnectionCallback);
    server.setMessageCallback(serverMessageCallback);
    server.start();
    loop.loop();
}

TcpConnectionPtr clientConnection;

void clientConnectionCallback(const TcpConnectionPtr& conn) {
    std::string state = (conn->connected() ? "UP" : "DOWN");
    printf("DiscardServer - %s -> %s is %s\n", conn->peerAddress().toIpPort().c_str(), 
    conn->localAddress().toIpPort().c_str(), state.c_str());

    if (conn->connected()) {
        clientConnection = conn;
        conn->setTcpNoDelay(true);
    } else {
        clientConnection.reset();
    }
}

void clientMessageCallback(const TcpConnectionPtr&, Buffer* buffer, Timestamp receiveTime) {
    int64_t message[2];
    while (buffer->readableBytes() >= frameLen) {
        memcpy(message, buffer->peek(), frameLen);
        buffer->retrieve(frameLen);

        int64_t send = message[0];
        int64_t their = message[1];
        int64_t back = receiveTime.microSecondsSinceEpoch();
        int64_t mine = (back + send) / 2;

        printf("round trip %d clock error %d\n", (int)(back - send), (int)(their - mine));
    }
}

void sendMyTime() {
    if (clientConnection) {
        int64_t message[2] = {0, 0};
        message[0] = Timestamp::now().microSecondsSinceEpoch();
        clientConnection->send(message, sizeof(message));
    }
}

void runClient(const char* ip, uint16_t port) {
    EventLoop loop;
    TcpClient client(&loop, InetAddress(ip, port), "ClockClient");

    client.enableRetry();
    client.setConnectionCallback(clientConnectionCallback);
    client.setMessageCallback(clientMessageCallback);
    client.connect();

    loop.runEvery(0.2, sendMyTime);
    loop.loop();
}

int main(int argc, char* argv[]) {

    if (argc > 2) {
        uint64_t port = static_cast<uint16_t>(atoi(argv[2]));
        if (strcmp(argv[1], "-s") == 0) {
            runServer(port);
        } else {
            runClient(argv[1], port);
        }
    } else {
        printf("Usage:\n%s -s port\n%s ip port\n", argv[0], argv[0]);
    }
    return 0;
}