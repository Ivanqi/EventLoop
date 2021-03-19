#ifndef EVENT_TEST_RESOLVER_H
#define EVENT_TEST_RESOLVER_H

#include "StringPiece.h"
#include "Timestamp.h"
#include "InetAddress.h"

#include <ares.h>
#include <functional>
#include <map>
#include <memory>

extern "C"
{
    struct hostent;
    struct ares_channeldata;
    // typedef struct ares_channledata *ares_channel;
};

class Channel;
class EventLoop;

class Resolver
{
    public:
        typedef std::function<void(const InetAddress&)> Callback;
        enum Option
        {
            kDNSandHostsFile,
            kDNSonly
        };

    private:
        Resolver *owner;
        Callback callback;
        EventLoop *loop_;
        ares_channel ctx_;
        bool timerActive_;
        typedef std::map<int, std::unique_ptr<Channel>> ChannelList;
        ChannelList channels_;

        struct QueryData
        {
            Resolver *owner;
            Callback callback;
            QueryData(Resolver *o, const Callback& cb)
                :owner(o), callback(cb)
            {

            }
        };
    
    public:
        explicit Resolver(EventLoop *loop, Option opt = kDNSandHostsFile);

        ~Resolver();

        bool resolve(StringArg hostname, const Callback& cb);
    
    private:
        void onRead(int sockfd, Timestamp t);

        void onTimer();

        void onQueryResult(int status, struct hostent *result, const Callback& cb);

        void onSockCreate(int sockfd, int type);

        void onSockStateChange(int sockfd, bool read, bool write);

        static void ares_host_callback(void *data, int status, int timeouts, struct hostent *hostent);

        static int ares_sock_create_callback(int sockfd, int type, void *data);

        static void ares_sock_state_callback(void *data, int sockfd, int read, int write);
};

#endif