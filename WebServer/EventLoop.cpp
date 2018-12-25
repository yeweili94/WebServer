#include "EventLoop.h"
#include "Channel.h"
#include "Poller.h"
#include "EPollPoller.h"
#include "base/Logging.h"

#include <poll.h>

using namespace ywl;
using namespace ywl::net;

namespace
{
__thread EventLoop* t_loopInThisThread = 0;
const int kPollTimeMs = 10000;
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
      currentActiveChannel_(NULL)
{
    LOG <<"EventLoop created " << this << "in thread " << threadId_;

    if (t_loopInThisThread)
    {
        FATAL << "Another EventLoop " << t_loopInThisThread
            << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
}

EventLoop::~EventLoop()
{
    t_loopInThisThread = NULL;
}

void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        /* wakeup(); */
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
    }
    
    LOG << "EventLoop " << this << " stop looping";
    looping_ = false;
}

void EventLoop::abortNotInLoopThread()
{
    FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
          << " was created in threadId_ = " << threadId_
          << ", current thread id = " << CurrentThread::tid();
    exit(-1);
}



