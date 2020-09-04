#include "EventLoop.h"
#include "MutexLock.h"
#include "Channel.h"
#include "Poller.h"
#include "SocketsOps.h"
#include "TimerQueue.h"

#include <algorithm>
#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdio.h>

__thread EventLoop *t_loopInThisThread = 0;

const int kPollTimeMs = 10000;

int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        printf("Failed in eventfd\n");
        abort();
    }
    return evtfd;
}

#pragma GCC diagnostic ignored "-Wold-stype-cast"
class IgnoreSigPipe
{
    public:
        IgnoreSigPipe()
        {
            ::signal(SIGPIPE, SIG_IGN);
        }
};
#pragma GCC diagnostic error "-Wold-stype-cast"

IgnoreSigPipe initObj;

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInThisThread;   
}

EventLoop::EventLoop()
    : looping_(false), quit_(false), eventHandling_(false), iteration_(0),
    threadId_(CurrentThread::tid()), poller_(Poller::newDefaultPoller(this)),
    timerQueue_(new TimerQueue(this)), wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this, wakeupFd_)), currentActiveChannel_(NULL)
{
    if (t_loopInThisThread) {
        printf("t_loopInThisThread is not null, exists in this thread: %d", threadId_);
    } else {
        t_loopInThisThread = this;
    }

    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;

    while (!quit_) {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        ++iteration_;

        eventHandling_ = true;
        for (Channel *channel: activeChannels_) {
            currentActiveChannel_ = channel;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }

        currentActiveChannel_ = NULL;
        eventHandling_ = false;
        doPendingFunctors();
    }

    looping_ = false;
}