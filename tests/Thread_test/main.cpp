#include "Thread.h"
#include "CurrentThread.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>

void mysleep(int seconds) {

    timespec t = {seconds, 0};
    nanosleep(&t, NULL);
}

void threadFunc() {

    printf("tid=%d\n", CurrentThread::tid());
}

void threadFunc2(int x) {

    printf("tid=%d, x = %d\n", CurrentThread::tid(), x);
}

void threadFunc3() {

    printf("tid=%d\n", CurrentThread::tid());
    mysleep(1);
}

class Foo
{
    private:
        double x_;

    public:
        explicit Foo(double x) : x_(x)
        {
        }

        void memberFunc()
        {
            printf("tid = %d, Foo:x_=%f\n", CurrentThread::tid(), x_);
        }

        void memberFunc2(const std::string& text)
        {
            printf("tid = %d, Foo::x_=%f, text=%s\n", CurrentThread::tid(), x_, text.c_str());
        }
};

int main() {

    printf("pid = %d, tid = %d\n", ::getpid(), CurrentThread::tid());

    Thread t1(threadFunc);
    t1.start();
    printf("t1.tid= %d\n", t1.tid());
    t1.join();

    printf("\n ------------------ \n");

    Thread t2(std::bind(threadFunc2, 42), "thread for free function with argument");
    t2.start();
    printf("t2.tid= %d\n", t2.tid()); 
    t2.join();

    printf("\n ------------------ \n");

    Foo foo(87.53);
    Thread t3(std::bind(&Foo::memberFunc, &foo), "thread for member function without argument");
    t3.start();
    t3.join();

    printf("\n ------------------ \n");

    Thread t4(std::bind(&Foo::memberFunc2, std::ref(foo), std::string("Ivan Qi")));
    t4.start();
    t4.join();

    printf("\n ------------------ \n");

    {
        Thread t5(threadFunc3);
        t5.start(); // t5可能比线程创建更容易破坏
    }

    mysleep(2);

    {
        Thread t6(threadFunc3);
        t6.start();
        mysleep(2); // t6在线程创建之后销毁
    }

    sleep(2);

    printf("number of created threads %d\n", Thread::numCreated());

    return 0;
}