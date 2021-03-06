#include <memory>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <linux/unistd.h>
#include <stdint.h>
#include <assert.h>
#include <iostream>

#include "Thread.h"
#include "CurrentThread.h"
#include "Exception.h"
#include "Timestamp.h"

pid_t gettid()
{
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

void CurrentThread::cacheTid()
{
    if (t_cachedTid == 0) {
        t_cachedTid = gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof(t_tidString), "%5d", t_cachedTid);
    }
}

void CurrentThread::sleepUsec(int64_t usec)
{
    struct timespec ts = {0, 0};
    ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond * 1000);
    ::nanosleep(&ts, NULL);
}

// 为了在线程中保留name,tid这些数据
struct ThreadData
{
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;
    std::string name_;
    pid_t* tid_;
    CountDownLatch* latch_;

    ThreadData(const ThreadFunc &func, const std::string& name, pid_t *tid, CountDownLatch *latch)
        :func_(func),
        name_(name),
        tid_(tid),
        latch_(latch)
    {}

    void runInThread()
    {
        *tid_ = CurrentThread::tid();
        tid_ = NULL;
        latch_->countDown();
        latch_ = NULL;
        CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
        prctl(PR_SET_NAME, CurrentThread::t_threadName);

        try {
            func_();
            CurrentThread::t_threadName = "finished";

        } catch (const Exception& ex) {
            CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
            abort();

        } catch (const std::exception& ex) {
            CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            abort();

        } catch(...) {
            CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
            throw;
        }
    }
};

void *startThread(void *obj)
{
    ThreadData* data = static_cast<ThreadData*>(obj);
    data->runInThread();
    delete data;
    return NULL;
}

AtomicInt32 Thread::numCreated_;

Thread::Thread(const ThreadFunc &func, const std::string &n)
    :started_(false),
    joined_(false),
    pthreadId_(0),
    tid_(0),
    func_(func),
    name_(n),
    latch_(1)
{
    setDefaultName();
}

Thread::~Thread()
{
    std::cout << "~Thread" << std::endl;
    if (started_ && !joined_) {
        // pthread_join()函数的替代函数
        // pthread_detach 即主线程与子线程分离，子线程结束后，资源自动回收
        pthread_detach(pthreadId_);
    }
}

void Thread::setDefaultName()
{
    int num = numCreated_.incrementAndGet();
    if (name_.empty()) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}

void Thread::start()
{
    assert(!started_);
    started_ = true;
    ThreadData* data = new ThreadData(func_, name_, &tid_, &latch_);
    if (pthread_create(&pthreadId_, NULL, &startThread, data)) {
        started_ = false;
        delete data;
    } else {
        latch_.wait();
        assert(tid_ > 0);
    }
}

int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    // pthread_join即是子线程合入主线程，主线程阻塞等待子线程结束，然后回收子线程资源
    return pthread_join(pthreadId_, NULL);
}