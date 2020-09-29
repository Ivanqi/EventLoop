#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <map>

#include <stdio.h>
#include <unistd.h>
#include <sys/timerfd.h>

using namespace std;

void print(const char *msg) {

    static map<const char*, Timestamp> lasts;
    Timestamp &last = lasts[msg];
    Timestamp now = Timestamp::now();

    printf("%s tid %d %s delay %f \n", now.toString().c_str(), CurrentThread::tid(), msg, timeDifference(now, last));

    last = now;
}

int createTimerfd();

void readTimerfd(int timerfd, Timestamp now);


// 使用相对时间，以适应挂钟变化
class PeriodicTimer
{
    private:
        EventLoop *loop_;
        const int timerfd_;
        Channel timerfdChannel_;
        const double interval_; // in seconds
        TimerCallback cb_;

    public:
        PeriodicTimer(EventLoop *loop, double interval, const TimerCallback& cb)
            : loop_(loop), timerfd_(createTimerfd()),
            timerfdChannel_(loop, timerfd_), interval_(interval), cb_(cb)
        {
            timerfdChannel_.setReadCallback(bind(&PeriodicTimer::handleRead, this));
            timerfdChannel_.enableReading();
        }

        void start()
        {
            struct itimerspec spec;
            memZero(&spec, sizeof(spec));
            spec.it_interval = toTimeSpec(interval_);
            spec.it_value = spec.it_interval;

            int ret = ::timerfd_settime(timerfd_, 0 /* relative timer */, &spec, NULL);

            if (ret < 0) {
                printf("PeriodicTimer start timerfd_settime() error\n");
            }
        }

        ~PeriodicTimer()
        {
            timerfdChannel_.disableAll();
            timerfdChannel_.remove();
            ::close(timerfd_);
        }
    
    private:
        void handleRead()
        {
            loop_->assertInLoopThread();
            readTimerfd(timerfd_, Timestamp::now());
            if (cb_) {
                cb_();
            }
        }

        static struct timespec toTimeSpec(double seconds)
        {
            struct timespec ts;
            memZero(&ts, sizeof(ts));

            const int64_t kNanoSecondsPerSecond = 1000000000;
            const int kMinInterval = 100000;

            int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);

            if (nanoseconds < kMinInterval) {
                nanoseconds = kMinInterval;
            }

            ts.tv_sec = static_cast<time_t>(nanoseconds / kNanoSecondsPerSecond);
            ts.tv_nsec = static_cast<long>(nanoseconds % kNanoSecondsPerSecond);

            return ts;
        }
};

int main() {

    printf("pid = %d, tid = %d Try adjusting the wall clock, see what happens.", getpid(), CurrentThread::tid());

    EventLoop loop;

    PeriodicTimer timer(&loop, 1, bind(print, "PeriodicTimer"));
    timer.start();

    loop.runEvery(1, bind(print, "EventLoop::runEvenry"));

    loop.loop();
}