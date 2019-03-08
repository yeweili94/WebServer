#include <WebServer/EventLoop.h>
#include <WebServer/Channel.h>
#include <WebServer/base/Logging.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/implicit_cast.hpp>

#include <signal.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

using namespace ywl;
using namespace ywl::net;

namespace
{
// ��ǰ�߳�EventLoop����ָ��
// �ֲ߳̾��洢
__thread EventLoop* t_loopInThisThread = 0;

const int KNew = -1;
const int KAdded = 1;
const int KDeleted = 2;

const int kPollTimeMs = 100000;

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

void IgnoreSIGPIPE()
{
    ::signal(SIGPIPE, SIG_IGN);
}

}//namespace

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
    epollfd_(epoll_create1(EPOLL_CLOEXEC)),
    epollEvents_(16),
    timerQueue_(new TimerManager(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this, wakeupFd_)),
    currentActiveChannel_(NULL)
{
    IgnoreSIGPIPE();
    if (epollfd_ < 0) 
    {
        FATAL << "epoll_create() failed!";
    }
    LOG << "EventLoop created " << this << " in thread " << threadId_;
    // �����ǰ�߳��Ѿ�������EventLoop������ֹ(LOG_FATAL)
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
    ::close(epollfd_);
    t_loopInThisThread = NULL;
}

// �¼�ѭ�����ú������ܿ��̵߳���
// ֻ���ڴ����ö�����߳��е���
void EventLoop::loop()
{
    assert(!looping_);
    // ��ǰ���ڴ����ö�����߳���
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    LOG << "EventLoop " << this << " start looping";

    while (!quit_)
    {
        activeChannels_.clear();
        int64_t nextExpiredTime = timerQueue_->nextExpired();
        pollReturnTime_ = poll(nextExpiredTime);
        eventHandling_ = true;
        for (auto it = activeChannels_.begin();
            it != activeChannels_.end(); ++it)
        {
            currentActiveChannel_ = *it;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = NULL;
        eventHandling_ = false;
        doPendingFunctors();
        timerQueue_->handleExpired();
    }

    LOG << "EventLoop " << this << " stop looping";
    looping_ = false;
}

// �ú������Կ��̵߳���
void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}

// ��I/O�߳���ִ��ĳ���ص��������ú������Կ��̵߳���
void EventLoop::runInLoop(const Functor& cb)
{
    if (isInLoopThread())
    {
      // ����ǵ�ǰIO�̵߳���runInLoop����ͬ������cb
        cb();
    }
    else
    {
      // ����������̵߳���runInLoop�����첽�ؽ�cb��ӵ�����
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const Functor& cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(cb);
    }
    // ����queueInLoop���̲߳���IO�߳���Ҫ����
    // ���ߵ���queueInLoop���߳���IO�̣߳����Ҵ�ʱ���ڵ���pending functor����Ҫ����
    // ֻ��IO�̵߳��¼��ص��е���queueInLoop�Ų���Ҫ����
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    epollUpdateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    if (eventHandling_)
    {
        assert(currentActiveChannel_ == channel ||  \
                 std::find(activeChannels_.begin(), \
                 activeChannels_.end(), channel) == activeChannels_.end());
    }
    epollRemoveChannel(channel);
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
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
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

void EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
	return timerQueue_->addTimer(cb, time, 0.0);
}

void EventLoop::runAfter(double delay, const TimerCallback& cb)
{
	Timestamp time(addTime(Timestamp::now(), delay));
	return runAt(time, cb);
}

void EventLoop::runEvery(double interval, const TimerCallback& cb)
{
	Timestamp time(addTime(Timestamp::now(), interval));
	return timerQueue_->addTimer(cb, time, interval);
}

Timestamp EventLoop::poll(int timeoutMs)
{
    int numEvents = ::epoll_wait(epollfd_, &*epollEvents_.begin(),
                                 static_cast<int>(epollEvents_.size()),
                                 timeoutMs);
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOG << numEvents << " events happend";
        fillActiveChannels(numEvents);
        if (boost::implicit_cast<size_t>(numEvents) == epollEvents_.size())
        {
            epollEvents_.resize(epollEvents_.size()*2);
        }
    }
    else if (numEvents == 0)
    {
        LOG << "nothing happend";
    }
    else
    {
        LOG << "EPollPoller::poll()";
    }
    return now;
}

void EventLoop::fillActiveChannels(int numEvents)
{
    assert(implicit_cast<size_t>(numEvents) <= events_.size());
    for (int i = 0; i < numEvents; i++)
    {
        Channel* channel = static_cast<Channel*>(epollEvents_[i].data.ptr);
        channel->set_revents(epollEvents_[i].events);
        activeChannels_.push_back(channel);
    }
}

void EventLoop::epollUpdateChannel(Channel* channel)
{
    assertInLoopThread();
    LOG << "fd = " << channel->fd() << " events = " << channel->events();
    const int status = channel->status();
    if (status == KNew)
    {
        channel->setStatus(KAdded);
        epollUpdate(EPOLL_CTL_ADD, channel);
    }
    else
    {
        if (channel->isNoneEvent())
        {
            epollUpdate(EPOLL_CTL_DEL, channel);
            channel->setStatus(KDeleted);
        }
        else
        {
            epollUpdate(EPOLL_CTL_MOD, channel);
        }
    }
}

void EventLoop::epollUpdate(int ope, Channel* channel)
{
    struct epoll_event ev;
    bzero(&ev, sizeof ev);
    ev.data.ptr = channel;
    ev.events = channel->events();
    int fd = channel->fd();

    int ret = ::epoll_ctl(epollfd_, ope, fd, &ev);
    if (ret < 0)
    {
        fprintf(stderr, "epoll_ctl wrong, code:%d\n", errno);
        if (ope == EPOLL_CTL_DEL)
        {
            LOG << "epoll_ctl ope = " << ope << " fd = " << fd;
        }
        else
        {
            FATAL << "epoll_ctl ope = " << ope << " fd = " << fd;
        }
    }
}

void EventLoop::epollRemoveChannel(Channel* channel)
{
    assertInLoopThread();
    int fd = channel->fd();
    LOG << "fd = " << fd;
    assert(channel->isNoneEvent());
    int status = channel->status();
    assert(status == KAdded);
    (void)status;

    if (channel->status() == KAdded)
    {
        epollUpdate(EPOLL_CTL_DEL, channel);
    }
    channel->setStatus(KNew);
}

