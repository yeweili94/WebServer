#include "../EventLoop.h"

using namespace ywl;
using namespace ywl::net;

EventLoop* g_loop;

void print()
{
    printf("love you\n");
}

void threadFunc()
{
    sleep(1);
    g_loop->runInLoop(print);
    sleep(1);
    g_loop->quit();
}

int main()
{
    EventLoop loop;
    g_loop = &loop;
    Thread t(threadFunc);
    t.start();
    loop.loop();
    t.join();
    return 0;
}
