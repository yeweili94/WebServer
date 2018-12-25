#include "../EventLoop.h"

using namespace ywl;
using namespace ywl::net;

EventLoop* g_loop;

void threadFunc()
{
    g_loop->loop();
}

int main()
{
    EventLoop loop;
    g_loop = &loop;
    Thread t(threadFunc);
    t.start();
    t.join();
    return 0;
}
