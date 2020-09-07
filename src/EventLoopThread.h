#ifndef EVENT_EVENTLOOPTHREAD_H
#define EVENT_EVENTLOOPTHREAD_H

#include "Condition.h"
#include "MutexLock.h"
#include "Thread.h"
#include <boost/noncopyable.hpp>

class EventLoop;

class EventLoopThread: boost::noncopyable
{
    public:
        typedef std::function<void(EventLoop*)> ThreadInitCallback;

    private:
        MutexLock mutex_;
        EventLoop *loop_;

        bool exiting_;
        Thread thread_;
        Condition cond_;
        ThreadInitCallback callback_;
    
    public:
        EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(), const strig& name = string());

        ~EventLoopThread();

        EventLoop *startLoop();
    
    private:
        void threadFunc();
};

#endif