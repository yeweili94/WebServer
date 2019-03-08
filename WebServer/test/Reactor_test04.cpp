#include <WebServer/Channel.h>
#include <WebServer/EventLoop.h>
#include <WebServer/base/Thread.h>

#include <boost/bind.hpp>

#include <stdio.h>
#include <unistd.h>

using namespace ywl;
using namespace ywl::net;

int cnt = 0;
EventLoop* g_loop;

void printTid()
{
    printf("pid=%d, tid=%d\n", getpid(), CurrentThread::tid());
    printf("now %s\n", Timestamp::now().toString().c_str());
}

void print(const char* msg)
{
    printf("msg %s %s\n", Timestamp::now().toString().c_str(), msg);
    if (++cnt == 20)
    {
        g_loop->quit();
    }
}

// void cancel(TimerId timer)
// {
//     g_loop->cancel(timer);
//     printf("cancelled at %s\n", Timestamp::now().toString().c_str());
// }

int main()
{
    printTid();
    sleep(1);
    {
        EventLoop loop;
        g_loop = &loop;

        print("main");
        loop.runAfter(1, boost::bind(print, "once1"));
        loop.runAfter(1.5, boost::bind(print, "once1.5"));
        loop.runAfter(2.5, boost::bind(print, "once2.5"));
        loop.runAfter(3.5, boost::bind(print, "once3.5"));
        // loop.runAfter(4.8, boost::bind(cancel, t45));
        loop.runEvery(2, boost::bind(print, "every2"));
        loop.loop();
        print("main loop exits");
    }
}
