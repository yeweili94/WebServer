#include <WebServer/base/Mutex.h>
#include <WebServer/base/Condition.h>
#include <WebServer/base/Thread.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <functional>
#include <queue>
#include <vector>

using namespace ywl;

using Task = std::function<void()>;

class ThreadPool
{
public:

    ThreadPool(int numThread);
    ~ThreadPool();

    void start();
    void addTask(const Task& task);
    void stop();

private:

    void runInThread();
    Task take();
    int numThreads_;
    bool running_;
    std::queue<Task> taskQueue_;
    boost::ptr_vector<Thread> threads_;
    MutexLock mutex_;
    Condition cond_;
};

ThreadPool::ThreadPool(int numThread)
    : numThreads_(numThread),
      running_(false),
      mutex_(),
      cond_(mutex_)
{

}

ThreadPool::~ThreadPool()
{

}

void ThreadPool::start()
{
    threads_.resize(numThreads_);
    for (int i = 0; i < numThreads_; i++) 
    {
        threads_.push_back(new Thread(boost::bind(&ThreadPool::runInThread, this), " "));
        threads_[i].start();
    }
}

Task ThreadPool::take()
{
    MutexLockGuard lock(mutex_);
    Task task;
    while (taskQueue_.empty() && running_)
    {
        cond_.wait();
    }
    if (!taskQueue_.empty()) 
    {
        task = taskQueue_.front();
        taskQueue_.pop();
    }
    return task;
}

void ThreadPool::addTask(const Task& task)
{
    MutexLockGuard lock(mutex_);
    bool notify = false;
    if (taskQueue_.empty()) {
        notify = true;
    }
    taskQueue_.push(task);
    if (notify)
    {
        cond_.notify();
    }
}

void ThreadPool::stop()
{
    {
        MutexLockGuard lock(mutex_);
        running_ = false;
        cond_.notifyAll();
    }

    for (int i = 0; i < numThreads_; i++)
    {
        threads_[i].join();
    }
}

void ThreadPool::runInThread()
{
    Task task;
    while(running_)
    {
        task = take();
        if (task)
        {
            task();
        }
    }
}
