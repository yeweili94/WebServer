#ifndef WEBSERVER_DYNAMIC_THREADPOOL_H
#define WEBSERVER_DYNAMIC_THREADPOOL_H

#include <condition_variable>
#include <list>
#include <queue>
#include <mutex>
#include <memory>

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <WebServer/base/Thread.h>

namespace ywl
{

class DynamicThreadPool
{
public:
    explicit DynamicThreadPool(int reserve_threads);
    ~DynamicThreadPool();

    void Add(const boost::function<void()>& callback);

private:
    class DynamicThread {
     public:
         DynamicThread(DynamicThreadPool* pool);
         ~DynamicThread();
     private:
         DynamicThreadPool* pool_;
         Thread thread_;
         void ThreadFunc();
    };
    std::mutex mutex_;
    std::condition_variable cond_;
    std::condition_variable shutdown_cond_;
    bool shutdown_;
    std::queue<boost::function<void()>> callbacks_;
    int reserve_threads_;
    int nthreads_;
    int threads_waiting_;
    std::list<DynamicThread*> dead_threads_;

    void ThreadFunc();
    static void ReapThreads(std::list<DynamicThread*>* tlist);
};

}
#endif
