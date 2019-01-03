#include <WebServer/EventLoopThread.h>
#include <WebServer/EventLoop.h>

#include <boost/bind.hpp>

using namespace ywl;
using namespace ywl::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb)
    : loop_(NULL),
      exiting_(false),
      thread_(boost::bind(&EventLoopThread::threadFunc, this), "EventLoopThread"),
      mutex_(),
      cond_(mutex_),
      callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    loop_->quit();
    thread_.join();
}

EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.start());
    thread_.start();
    {
        MutexLockGuard lock(mutex_);
        while (loop_ == NULL) {
            cond_.wait();
        }
    }
    return loop_;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;
    if (callback_)
    {
        callback_(&loop);
    }
    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();
    }
    loop.loop();
    assert(exiting_);
    loop_ = NULL;
}

////////////////////////////////////////////////////////////////////////////////
EventLoopThreadPool::EventLoopThreadPool(EventLoop* mainLoop)
    : mainLoop_(mainLoop),
      numThreads_(0),
      next_(0),
      started_(false)
{

}

EventLoopThreadPool::~EventLoopThreadPool()
{
    //loop是在栈上创建的对象，不需要删除
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
    assert(!started_);
    mainLoop_->assertInLoopThread();
    started_ = true;
    for (int i = 0; i < numThreads_; i++) {
        EventLoopThread* th = new EventLoopThread(cb);
        threads_.push_back(th);
        loops_.push_back(th->startLoop());
    }
    if (numThreads_ == 0 && cb)
    {
        cb(mainLoop_);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    mainLoop_->assertInLoopThread();
    EventLoop* loop = mainLoop_;
    if (!loops_.empty())
    {
        loop = loops_[next_];
        ++next_;
        if (next_ == (int)loops_.size())
        {
            next_ = 0;
        }
    }
    return loop;
}

