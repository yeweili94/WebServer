#include "ThreadPool.h"

#include <boost/bind.hpp>

#include <assert.h>
#include <stdio.h>

using namespace ywl;

ThreadPool::ThreadPool(const std::string& name)
    : mutex_(),
      cond_(mutex_),
      name_(std::move(name)),
      running_(false)
{

}

ThreadPool::~ThreadPool()
{
    if (running_)
    {
        stop();
    }
}

void ThreadPool::start(int numThreads)
{
    assert(threads_.empty());
    running_ = true;
    threads_.reserve(numThreads);
    for (int i = 0; i < numThreads; i++)
    {
        char id[32];
        snprintf(id, sizeof(id)-1, "%d", i);
        threads_.push_back(new ywl::Thread(
                    boost::bind(&ThreadPool::runInThread, this), name_+id));
        threads_[i].start();
    }
}

void ThreadPool::stop()
{
    {
        MutexLockGuard lock(mutex_);
        running_ = false;
        cond_.notifyAll();
    }
    for (size_t i = 0; i < threads_.size(); i++)
    {
        threads_[i].join();
    }
}

void ThreadPool::run(const Task& task)
{
    if (threads_.empty())
    {
        task();
    }
    else
    {
        MutexLockGuard lock(mutex_);
        queue_.push_back(task);
        cond_.notify();
    }
}

ThreadPool::Task ThreadPool::take()
{
    MutexLockGuard lock(mutex_);
    while (queue_.empty() && running_)
    {
        cond_.wait();
    }
    Task task;
    if (!queue_.empty())
    {
        task = queue_.front();
        queue_.pop_front();
    }
    return task;
}

void ThreadPool::runInThread()
{
    //FIXME
    while (running_)
    {
        Task task(take());
        if (task)
        {
            task();
        }
    }
}


