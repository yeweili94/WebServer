#include "Thread.h"
#include "CurrentThread.h"

#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/unistd.h>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>


namespace ywl
{
namespace CurrentThread
{
    __thread int t_cachedTid = 0;
    __thread char t_tidString[32];
    __thread const char* t_threadName = "unknown";
    const bool sameType = boost::is_same<int, pid_t>::value;
    BOOST_STATIC_ASSERT(sameType);
    
}//namespace CurrentThread

namespace detail
{
    pid_t gettid()
    {
        return static_cast<pid_t>(::syscall(SYS_gettid));
    }

    void afterFork()
    {
        CurrentThread::t_cachedTid = 0;
        CurrentThread::t_threadName = "main";
        CurrentThread::tid();
    }

    class ThreadNameInitializer
    {
    public:
        ThreadNameInitializer()
        {
            CurrentThread::t_threadName = "main";
            CurrentThread::tid();
            pthread_atfork(NULL, NULL, &afterFork);
        }
    };

    //初始化主线程
    ThreadNameInitializer init;
}//namespace detail
}//namespace ywl


using namespace ywl;

void CurrentThread::cacheTid()
{
    if (t_cachedTid == 0) 
    {
        t_cachedTid = detail::gettid();
        int n = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
        assert(n == 6);
        (void) n;
    }
}

bool CurrentThread::isMainThread()
{
    return tid() == ::getpid();
}

std::atomic<int32_t> Thread::numCreated_;

Thread::Thread(const ThreadFunc& func, const std::string& name)
    : started_(false),
      pthreadId_(0),
      tid_(0),
      func_(func),
      name_(name)
{
    numCreated_.fetch_add(1);
}

Thread::~Thread()
{

}

void Thread::start()
{
    assert(!started_);
    started_ = true;
    errno = pthread_create(&pthreadId_, NULL, &startThread, this);
    if (errno != 0)
    {
        //FIXME
        perror("Fatal : Failed in pthread_create\n");
        exit(-1);
    }
}

int Thread::join()
{
    assert(started_);
    return pthread_join(pthreadId_, NULL);
}

void* Thread::startThread(void* obj)
{
    Thread* thread = static_cast<Thread*>(obj);
    thread->runInThread();
    return NULL;
}

void Thread::runInThread()
{
    tid_ = CurrentThread::tid();
    CurrentThread::t_threadName = name_.c_str();
    //FIXME
    func_();
    CurrentThread::t_threadName = "finished";
}

