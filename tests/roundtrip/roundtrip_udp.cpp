#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "SocketsOps.h"
#include "TcpClient.h"
#include "TcpServer.h"

#include <stdio.h>

const size_t frameLen = 2 * sizeof(int64_t);

int createNonblockingUDP() {
    int sockfd = ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);
    if (sockfd < 0) {
        printf("::socket\n");
    }
    return sockfd;
}

// Server

void serverReadCallback(int sockfd, Timestamp receivTime) {
    int64_t message[2];
    struct sockaddr peerAddr;
    memZero(&peerAddr, sizeof(peerAddr));
    socklen_t addrLen = sizeof(peerAddr);
    ssize_t nr = ::recvfrom(sockfd, message, sizeof(message), 0, &peerAddr, &addrLen);

    char addrStr[64];
    sockets::toIpPort(addrStr, sizeof(addrStr), &peerAddr);
    printf("received %d bytes from %s\n", (int)nr, addrStr);

    if (nr < 0) {
        printf("::recvfrom\n");
    } else if (implicit_cast<size_t>(nr) == frameLen) {
        message[1] = receivTime.microSecondsSinceEpoch();
        ssize_t nw = ::sendto(sockfd, message, sizeof(message), 0, &peerAddr, addrLen);

        if (nw < 0) {
            printf("::sendto\n");
        } else if (implicit_cast<size_t>(nw) != frameLen) {
            printf("Expect %d bytes, wrote %d bytes\n", (int)frameLen, (int)nw);
        }
    } else {
        printf("Expect %d bytes, received %d bytes.\n", (int)frameLen, (int)nr);
    }
}

void runServer(uint16_t port) {
    Socket sock(createNonblockingUDP());
    sock.bindAddress(InetAddress(port));

    EventLoop loop;
    Channel channle(&loop, sock.fd());
    channle.setReadCallback(std::bind(&serverReadCallback, sock.fd(), _1));
    channle.enableReading();
    
    loop.loop();
}

// Client

void clientReadCallback(int sockfd, Timestamp receiveTime) {
    int64_t message[2];
    ssize_t nr = sockets::read(sockfd, message, sizeof(message));

    if (nr < 0) {
        printf("::read\n");
    } else if (implicit_cast<size_t>(nr) == frameLen) {
        int64_t send = message[0];
        int64_t their = message[1];
        int64_t back = receiveTime.microSecondsSinceEpoch();
        int64_t mine = (back + send) / 2;

        printf("round trip %d clock error %d\n", (int)(back - send), (int)(their - mine));
    } else {
        printf("Expect %d bytes, received %d bytes.\n", (int)frameLen, (int)nr);
    }
}

void sendMyTime(int sockfd) {
    int64_t message[2] = {0, 0};
    message[0] = Timestamp::now().microSecondsSinceEpoch();
    size_t nw = sockets::write(sockfd, message, sizeof(message));

    if (nw < 0) {
        printf("::write\n");
    } else if (implicit_cast<size_t>(nw) != frameLen){
        printf("Expect %d bytes, received %d bytes.\n", (int)frameLen, (int)nw);
    }
}

void runClient(const char* ip, uint16_t port) {
    Socket sock(createNonblockingUDP());
    InetAddress serverAddr(ip, port);
    int ret = sockets::connect(sock.fd(), serverAddr.getSockAddr());

    if (ret < 0) {
        printf("::connect\n");
    }

    EventLoop loop;
    Channel channel(&loop, sock.fd());
    channel.setReadCallback(std::bind(&clientReadCallback, sock.fd(), _1));
    channel.enableReading();
    loop.runEvery(0.2, std::bind(sendMyTime, sock.fd()));
    loop.loop();
}

int main(int argc, char* argv[]) {

    if (argc > 2) {
        uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
        if (strcmp(argv[1], "-s") == 0) {
            runServer(port);
        } else {
            runClient(argv[1], port);
        }
    } else {
        printf("Usage:\n%s -s port \n %s ip port\n", argv[0], argv[0]);
    }
    return 0;
}