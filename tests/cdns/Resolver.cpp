#include "Resolver.h"
#include "Channel.h"
#include "EventLoop.h"

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

double getSeconds(struct timeval* tv)
{
    if (tv) {
        return double(tv->tv_sec) + double(tv->tv_usec) / 1000000.0;
    } else {
        return -1.0;
    }
}

const char *getSocketType(int type)
{
    if (type == SOCK_DGRAM) {
        return "UDP";
    } else if (type == SOCK_STREAM) {
        return "TCP";
    } else {
        return "Unknown";
    }
}

const bool kDebug = false;

Resolver::Resolver(EventLoop *loop, Option opt)
    :loop_(loop), ctx_(NULL), timerActive_(false)
{
    static char lookups[] = "b";
    int optmask = ARES_OPT_FLAGS;

    struct ares_options options;
    options.flags = ARES_FLAG_NOCHECKRESP;
    options.flags |= ARES_FLAG_STAYOPEN;
    options.flags |= ARES_FLAG_IGNTC; // UDP only
    optmask |= ARES_OPT_SOCK_STATE_CB;
    options.sock_state_cb = &Resolver::ares_sock_state_callback;
    options.sock_state_cb_data = this;
    optmask |= ARES_OPT_TIMEOUT;
    options.timeout = 2;

    if (opt == kDNSonly) {
        optmask |= ARES_OPT_LOOKUPS;
        options.lookups = lookups;
    }

    int status = ares_init_options(&ctx_, &options, optmask);
    if (status != ARES_SUCCESS) {
        assert(0);
    }

    ares_set_socket_callback(ctx_, &Resolver::ares_sock_create_callback, this);
}

Resolver::~Resolver()
{
    ares_destroy(ctx_);
}

bool Resolver::resolve(StringArg hostname, const Callback& cb)
{
    loop_->assertInLoopThread();
    QueryData *queryData = new QueryData(this, cb);
    ares_gethostbyname(ctx_, hostname.c_str(), AF_INET, &Resolver::ares_host_callback, queryData);

    struct timeval tv;
    struct timeval *tvp = ares_timeout(ctx_, NULL, &tv);
    double timeout = getSeconds(tvp);

    printf("timeout %lf active %d\n", timeout, timerActive_);
    if (!timerActive_) {
        loop_->runAfter(timeout, std::bind(&Resolver::onTimer, this));
        timerActive_ = true;
    }

    return queryData != NULL;
}

void Resolver::onRead(int sockfd, Timestamp t)
{
    printf("onRead %d at %s\n", sockfd, t.toString().c_str());
    ares_process_fd(ctx_, sockfd, ARES_SOCKET_BAD);
}

void Resolver::onTimer()
{
    assert(timerActive_ == true);
    ares_process_fd(ctx_, ARES_SOCKET_BAD, ARES_SOCKET_BAD);

    struct timeval tv;
    struct timeval *tvp = ares_timeout(ctx_, NULL, &tv);
    double timeout = getSeconds(tvp);

    printf("%s next timeout %lf\n", loop_->pollReturnTime().toString().c_str(), timeout);

    if (timeout < 0) {
        timerActive_ = false;
    } else {
        loop_->runAfter(timeout, std::bind(&Resolver::onTimer, this));
    }
}

void Resolver::onQueryResult(int status, struct hostent* result, const Callback& callback)
{
    printf("onQueryResult %d\n", status);
    struct sockaddr_in addr;

    memZero(&addr, sizeof(addr)); 
    addr.sin_family = AF_INET;
    addr.sin_port = 0;

    if (result) {
        addr.sin_addr = *reinterpret_cast<in_addr*>(result->h_addr);
        if (kDebug) {
            printf("h_name %s\n", result->h_name);
            for (char **alias = result->h_aliases; *alias != NULL; ++alias) {
                printf("alias: %s\n", *alias);
            }

            for (char **haddr = result->h_addr_list; *haddr != NULL; ++haddr) {
                char buf[32];
                inet_ntop(AF_INET, *haddr, buf, sizeof(buf));
                printf("    %s\n", buf);
            }
        }
    }

    InetAddress inet(addr);
    callback(inet);
}

void Resolver::onSockCreate(int sockfd, int type)
{
    loop_->assertInLoopThread();
    assert(channels_.find(sockfd) == channels_.end());

    Channel *channel = new Channel(loop_, sockfd);
    channel->setReadCallback(std::bind(&Resolver::onRead, this, sockfd, _1));
    channel->enableReading();
    channels_[sockfd].reset(channel);
}

void Resolver::onSockStateChange(int sockfd, bool read, bool write)
{
    loop_->assertInLoopThread();
    ChannelList::iterator it = channels_.find(sockfd);
    assert(it != channels_.end());

    if (read) {

    } else {
        it->second->disableAll();
        it->second->remove();
        channels_.erase(it);
    }
}

void Resolver::ares_host_callback(void* data, int status, int timeouts, struct hostent* hostent)
{
    QueryData* query = static_cast<QueryData*>(data);

    query->owner->onQueryResult(status, hostent, query->callback);
    delete query;
}

int Resolver::ares_sock_create_callback(int sockfd, int type, void *data)
{
    printf("sockfd = %d type = %s\n", sockfd, getSocketType(type));
    static_cast<Resolver*>(data)->onSockCreate(sockfd, type);
    return 0;
}

void Resolver::ares_sock_state_callback(void* data, int sockfd, int read, int write)
{
    printf("sockfd = %d read = %d write=%d\n", sockfd, read, write);
    static_cast<Resolver*>(data)->onSockStateChange(sockfd, read, write);
}
