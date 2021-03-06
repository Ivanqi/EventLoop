#include "EventLoopThread.h"
#include "EventLoop.h"
#include "Thread.h"
#include "CountDownLatch.h"

#include <stdio.h>
#include <unistd.h>

void print(EventLoop *p = NULL) {

    printf("print: pid = %d, tid = %d, loop = %p\n", getpid(), CurrentThread::tid(), p);
}

void quit(EventLoop *p) {

    print(p);
    p->quit();
}

int main() {

    print();

    {
        EventLoopThread thr1;   // never start
    }
    printf("\n ------------------- \n");
    {
        // dtor calss quit()
        EventLoopThread thr2;
        EventLoop *loop = thr2.startLoop();
        loop->runInLoop(std::bind(print, loop));
        CurrentThread::sleepUsec(500 * 1000);
    }
    printf("\n ------------------- \n");
    {
        // quit() before dtor
        EventLoopThread thr3;
        EventLoop *loop = thr3.startLoop();
        loop->runInLoop(std::bind(quit, loop));
        CurrentThread::sleepUsec(500 * 1000);
    }

    return 0;
}