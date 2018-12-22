#include "../ThreadPool.h"
#include "../CountDownLatch.h"
#include "../CurrentThread.h"

#include <boost/bind.hpp>
#include <stdio.h>

void print()
{
    printf("tid=%d\n", ywl::CurrentThread::tid());
}

void printString(const std::string& str)
{
    printf("tid=%d, str=%s\n", ywl::CurrentThread::tid(), str.c_str());
}

int main()
{
    ywl::ThreadPool pool("MainThreadPool");
    pool.start(5);

    pool.run(print);
    pool.run(print);
    for (int i = 0; i < 100; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof buf, "task %d", i);
        pool.run(boost::bind(printString, std::string(buf)));
    }

    ywl::CountDownLatch latch(1);
    pool.run(boost::bind(&ywl::CountDownLatch::countDown, &latch));
    latch.wait();
    pool.stop();
}
