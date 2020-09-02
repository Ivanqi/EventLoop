#ifndef EVENT_SOCKETSOPS_H
#define EVENT_SOCKETSOPS_H

#include <arpa/inet.h>

namespace sockets
{

// 创建一个非阻塞的套接字文件描述符，如果有任何错误，则中止
int createNonblockingOrDie(sa_family_t family);

int connect(int sockfd, const struct sockaddr* addr);

int bindOrDie(int sockfd, const struct sockaddr* addr);

void listenOrDie(int sockfd);

int accept(int sockfd, struct sockaddr_in6* addr);

ssize_t read(int sockfd, void *buf, size_t count);

ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt);

ssize_t write(int sockfd, const void *buf, size_t count);

void close(int sockfd);

void shutdownWrite(int sockfd);

void toIpPort(char* buf, size_t size, const struct sockaddr* addr);

void toIp(char *buf, size_t size, const struct sockaddr* addr);

void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr);

void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr);

int getSocketError(int sockfd);

const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);

const struct sockaddr* sockaddr_case(const struct sockaddr_in6* addr);

struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr);

const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);

const struct sockadrr_in6* sockaddr_in6_cast(const struct sockaddr* addr);

struct sockaddr_in6 getLocalAddr(int sockfd);

struct sockaddr_in6 getPeerAddr(int sockfd);

bool isSelfConnect(int sockfd);

};

#endif