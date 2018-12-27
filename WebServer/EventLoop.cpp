#include <WebServer/EventLoop.h>
#include <WebServer/Channel.h>
#include <WebServer/Poller.h>
#include <WebServer/EPollPoller.h>
#include <WebServer/base/Logging.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <poll.h>
#include <sys/eventfd.h>

using namespace ywl;
using namespace ywl::net;

namespace
{
__thread EventLoop* t_loopInThisThread = 0;
const int kPollTimeMs = 10000;

int createEventfd()
{
    int etfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (etfd < 0)
    {
        FATAL << "Failed in create eventfd";
    }
    return etfd;
}

}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInThisThread;
}

EventLoop::EventLoop()
    : quit_(false),
      looping_(false),
      eventHandling_(false),
      threadId_(CurrentThread::tid()),
      poller_(new EPollPoller(this)),
      currentActiveChannel_(NULL),
      mutex_(),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG << "EventLoop created " << this << " in thread " << threadId_;

    if (t_loopInThisThread)
    {
        FATAL << "Another EventLoop " << t_loopInThisThread
            << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(boost::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    t_loopInThisThread = NULL;
}

//避免busy loop, 一定要把可读事件处理掉
void EventLoop::handleRead()
{
    uint64_t onebyte = 1;
    ssize_t n = ::read(wakeupFd_, &onebyte, sizeof onebyte);
    if (n != sizeof onebyte)
    {
        LOG << "ERROR-EventLoop::handleRead() reads " << n << " bytes not 8";
    }
}

void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    if (eventHandling_)
    {
        assert(currentActiveChannel_ == channel||
               std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_= true;
    LOG << "EventLoop " << this << " start looping";

    while (!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);

        eventHandling_ = true;
        for (auto it = activeChannels_.begin();
                it != activeChannels_.end(); ++it)
        {
            currentActiveChannel_ = *it;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = NULL;
        eventHandling_ = false;
        dopendingFunctors();
    }
    
    LOG << "EventLoop " << this << " stop looping";
    looping_ = false;
}

void EventLoop::abortNotInLoopThread()
{
    FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
          << " was created in threadId_ = " << threadId_
          << ", current thread id = " << CurrentThread::tid();
}

//该函数可以跨线程调用，在I/O线程中执行回调函数
void EventLoop::runInLoop(const Functor& cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const Functor& cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(cb);
    }
    //当调用queueInLoop的线程不是I/O线程时需要唤醒
    //当调用queueInLoop的线程是I/O线程，并且此时正在调用pendingFunctor，需要唤醒
    //在I/O线程的事件回调中调用queueInLoop才不用唤醒线程
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

//产生可读事件唤醒loop在epoll_wait处的阻塞
//向wakeupFd_写入事件
void EventLoop::wakeup()
{
    uint64_t onebyte = 1;
    int n = ::write(onebyte, &wakeupFd_, sizeof onebyte);
    if (n != sizeof onebyte)
    {
        LOG << "ERROR-EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::dopendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for (size_t i = 0; i < functors.size(); i++)
    {
        functors[i]();
    }
    callingPendingFunctors_ = false;
}
