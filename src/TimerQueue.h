#ifndef EVENT_TIMERQUEUE_H
#define EVENT_TIMERQUEUE_H

#include <set>
#include <vector>

#include "MutexLock.h"
#include "Timestamp.h"
#include "Callbacks.h"
#include "Channel.h"

class EventLoop;
class Timer;
class TimerId;

/**
 * 最大努力计时器队列
 * 不能保证回调会准时
 */
class TimerQueue
{
    private:
        /**
         * 使用unique_ptr<Timer>而不是原始指针
         */
        typedef std::pair<Timestamp, Timer*> Entry;
        typedef std::set<Entry> TimerList;
        typedef std::pair<Timer*, int64_t> ActiveTimer;
        typedef std::set<ActiveTimer> ActiveTimerSet;

        EventLoop* loop_;

        const int timerfd_;

        Channel timerfdChannel_;

        // 按过期排序的计时器列表
        TimerList timers_;

        // 取消
        ActiveTimerSet activeTimers_;
        bool callingExpiredTimers_; /* atomic */
         ActiveTimerSet cancelingTimers_;

    public:
        explicit TimerQueue(EventLoop *loop);
        ~TimerQueue();

        /**
         * 计划在给定时间运行回调，如果 interval>0.0，则重复
         * 线程安全的
         */
        void addTimer(TimerCallback cb, Timestamp when, double interval);

        void cancel(TimerId TimerId);
    
    private:
        void addTimerInLoop(Timer* timer);
        
        void cacelInLoop(TimerId timerId);

        // 当timerfd报警时调用
        void handleRead();

        // 移走所有过期的计时器
        std::vector<Entry> getExpired(Timestamp now);

        void reset(const std::vector<Entry>& expired, Timestamp now);

        bool insert(Timer* timer);
};

#endif