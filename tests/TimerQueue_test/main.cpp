#include "EventLoop.h"
#include "EventLoopThread.h"
#include "Thread.h"

#include <stdio.h>
#include <unistd.h>

int cnt = 0;
EventLoop *g_loop;

void printTid() {

    printf("pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("now %s \n", Timestamp::now().toString().c_str());
}

void print(const char* msg) {

    printf("msg %s %s\n", Timestamp::now().toString().c_str(), msg);
    if (++cnt == 20) {
        g_loop->quit();
    }
}

void cancel(TimerId timer) {

    g_loop->cancel(timer);
    printf("cancelled at %s \n", Timestamp::now().toString().c_str());
}

int main() {

    printTid();
    sleep(1);
    {
        EventLoop loop;
        g_loop = &loop;

        print("main");

        loop.runAfter(1, std::bind(print, "once1"));
    }
    return 0;
}