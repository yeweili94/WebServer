#include "Thread.h"

#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/unistd.h>

__thread pid_t t_cachedTid = 0;
pid_t gettid()
{
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

namespace CurrentThread
{
    __thread const char* t_threadName = "unknown";
    pid_t tid()
    {
        if (t_cachedTid == 0) {
            t_cachedTid = gettid();
        }
        return t_cachedTid;
    }

    const char* name()
    {
        return t_threadName;
    }

    bool isMainThread()
    {
        return tid() == ::getpid();
    }
}//namespace CurrentThread

class ThreadNameInitializer
{
public:
    ThreadNameInitializer()
    {
        CurrentThread::t_threadName = "main";
    }
};

ThreadNameInitializer init;

