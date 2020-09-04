#ifndef EVENT_POLLER_H
#define EVENT_POLLER_H

#include <map>
#include <vector>

#include "Timestamp.h"
#include "EventLoop.h"

class Channel;

// IO复用基础类
class Poller
{
    public:
        typedef std::vector<Channel*> ChannelList;
    
    protected:
        typedef std::map<int, Channel*> ChannelMap;
        ChannelMap channels_;
    
    private:
        EventLoop *ownerLoop_;
    
    public:
        Poller(EventLoop* loop);

        virtual ~Poller();

        /**
         * 轮询 I/O事件
         * 必须在循环线程中调用
         */
        virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

        /**
         * 更改关系的I/O事件
         * 必须在循环线程中调用
         */
        virtual void updateChannel(Channel *channel) = 0;

        /**
         * 当通道被破坏时，移除它
         * 必须在循环线程中调用
         */
        virtual void removeChannel(Channel *channel) = 0;

        virtual bool hasChannel(Channel *channel) const;

        static Poller* newDefaultPoller(EventLoop* loop);

        void assertInLoopThread() const
        {
            ownerLoop_->assertInLoopThread();
        }
};

#endif