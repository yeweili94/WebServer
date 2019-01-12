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
// 当前线程EventLoop对象指针
// 线程局部存储
__thread EventLoop* t_loopInThisThread = 0;

const int kPollTimeMs = 5000;

int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG << "Failed in eventfd";
        abort();
    }
    return evtfd;
}

}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInThisThread;
}

EventLoop::EventLoop()
  : looping_(false),
    quit_(false),
    eventHandling_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    poller_(new EPollPoller(this)),
    timerQueue_(new TimerManager(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this, wakeupFd_)),
    currentActiveChannel_(NULL)
{
    LOG << "EventLoop created " << this << " in thread " << threadId_;
    // 如果当前线程已经创建了EventLoop对象，终止(LOG_FATAL)
    if (t_loopInThisThread)
    {
        FATAL << "Another EventLoop " << t_loopInThisThread
                << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(
        boost::bind(&EventLoop::handleRead, this));
    // we are always reading the wakeupfd
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;
}

// 事件循环，该函数不能跨线程调用
// 只能在创建该对象的线程中调用
void EventLoop::loop()
{
    assert(!looping_);
    // 当前处于创建该对象的线程中
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    LOG << "EventLoop " << this << " start looping";

    while (!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        eventHandling_ = true;
        for (ChannelList::iterator it = activeChannels_.begin();
            it != activeChannels_.end(); ++it)
        {
            currentActiveChannel_ = *it;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = NULL;
        eventHandling_ = false;
        doPendingFunctors();
    }

    LOG << "EventLoop " << this << " stop looping";
    looping_ = false;
}

// 该函数可以跨线程调用
void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}

// 在I/O线程中执行某个回调函数，该函数可以跨线程调用
void EventLoop::runInLoop(const Functor& cb)
{
    if (isInLoopThread())
    {
      // 如果是当前IO线程调用runInLoop，则同步调用cb
        cb();
    }
    else
    {
      // 如果是其它线程调用runInLoop，则异步地将cb添加到队列
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const Functor& cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(cb);
    }

    // 调用queueInLoop的线程不是IO线程需要唤醒
    // 或者调用queueInLoop的线程是IO线程，并且此时正在调用pending functor，需要唤醒
    // 只有IO线程的事件回调中调用queueInLoop才不需要唤醒
    if (!isInLoopThread() || callingPendingFunctors_)
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
        assert(currentActiveChannel_ == channel ||
            std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
    LOG << "EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in threadId_ = " << threadId_
              << ", current thread id = " <<  CurrentThread::tid();
    abort();
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    //ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    //ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (size_t i = 0; i < functors.size(); ++i)
    {
        functors[i]();
    }
    callingPendingFunctors_ = false;
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
	return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
	Timestamp time(addTime(Timestamp::now(), delay));
	return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
	Timestamp time(addTime(Timestamp::now(), interval));
	return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
	return timerQueue_->cancel(timerId);
}

