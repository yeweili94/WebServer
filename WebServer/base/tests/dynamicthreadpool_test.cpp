#include <WebServer/base/DynamicThreadPool.h>
#include <WebServer/base/CountDownLatch.h>
#include <WebServer/base/CurrentThread.h>

#include <boost/bind.hpp>
#include <stdio.h>

void printString(const std::string& str)
{
    printf("tid=%d, str=%s\n", ywl::CurrentThread::tid(), str.c_str());
}

int main()
{
    ywl::DynamicThreadPool pool(20);

    for (int i = 0; i < 100; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof buf, "task %d", i);
        pool.Add(boost::bind(printString, std::string(buf)));
    }
}
