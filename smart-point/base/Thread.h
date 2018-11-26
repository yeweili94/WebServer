#ifndef MUDUO_STUDY_THREAD_H
#define MUDUO_STUDY_THREAD_H

#include <pthread.h>
#include <boost/function.hpp>

namespace CurrentThread
{
    pid_t tid();
    const char* name();
    bool isMainThread();
}

#endif

