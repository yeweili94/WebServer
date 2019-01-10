#include <WebServer/EventLoopThread.h>
#include <WebServer/EventLoop.h>

#include <stdio.h>
#include <boost/bind.hpp>

using namespace ywl;
using namespace ywl::net;

void funInthread()
{
    printf("runInThread():pid = %d;tid = %d\n", getpid(), CurrentThread::tid());
}

int main()
{
    printf("main() : pid = %d; tid = %d\n", getpid(), CurrentThread::tid());
    funInthread();
    EventLoopThread loopThread;
    EventLoop* loop = loopThread.startLoop();
    //异步调用runInLoop
    loop->runInLoop(boost::bind(funInthread));
    // sleep(2);
    //异步调用
    // loop->runAfter(2, boost::bind(funInthread));
    loop->runInLoop(boost::bind(funInthread));
    loop->runInLoop(boost::bind(funInthread));
    loop->runAfter(2, boost::bind(funInthread));
    loop->runEvery(3, boost::bind(funInthread));
    sleep(20);
    loop->quit();
    printf("main() exit\n");
}
