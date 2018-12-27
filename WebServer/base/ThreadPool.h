#ifndef WEB_SERVER_THREAD_POOL_H
#define WEB_SERVER_THREAD_POOL_H

#include <WebServer/base/Mutex.h>
#include <WebServer/base/Condition.h>
#include <WebServer/base/Thread.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <deque>

namespace ywl
{

class ThreadPool : public boost::noncopyable
{
public:
    using Task = boost::function<void ()>;

    explicit ThreadPool(const std::string& name = std::string());
    ~ThreadPool();

    void start(int numThreads);
    void run(const Task& f);
    void stop();

private:
    void runInThread();
    Task take();

    MutexLock mutex_;
    Condition cond_;
    std::string name_;
    boost::ptr_vector<Thread> threads_;
    std::deque<Task> queue_;
    bool running_;
};

}

#endif
