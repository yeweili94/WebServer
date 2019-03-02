#include <WebServer/Timer.h>

#include <WebServer/base/Logging.h>
#include <WebServer/EventLoop.h>

#include <boost/bind.hpp>
#include <sys/timerfd.h>

namespace ywl
{
namespace net
{
namespace detail
{

int createTimerfd()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                   TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0) {
        FATAL << "Failed in timerfd_create";
    }
    return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when)
{
    int64_t microseconds = when.microSecondsSinceEpoch()
                         - Timestamp::now().microSecondsSinceEpoch();
    microseconds = microseconds < 100 ? 100 : microseconds;

    struct timespec timespec;
    timespec.tv_sec = static_cast<time_t>(
            microseconds / Timestamp::KMicroSecondsPerSecond);
    timespec.tv_nsec = static_cast<long>(
            (microseconds % Timestamp::KMicroSecondsPerSecond) * 1000);
    return timespec;
}

//�����ʱ��������һֱ����
void readTimerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    LOG << "TimeManager::handleRead() " << howmany << " at " << now.toString();
    if (n != sizeof howmany)
    {
        LOG << "TimerManager::handleRead() reads " << n << " bytes instead of 8";
    }
}

//���ö�ʱ����ʱʱ��
void resetTimerfd(int timerfd, Timestamp expiration)
{
    //wake up loop by timerfd_settime()
    struct itimerspec newValue;
    struct itimerspec oldValue;
    bzero(&newValue, sizeof newValue);
    bzero(&oldValue, sizeof oldValue);
    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret) {
        LOG << "timerfd_settime()";
    }
}

}//namespace detail

using namespace detail;

TimerManager::TimerManager(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_(),
      callingExpiredTimers_(false)
{
    timerfdChannel_.setReadCallback(
        boost::bind(&TimerManager::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerManager::~TimerManager()
{
    ::close(timerfd_);
    for (TimerList::iterator it = timers_.begin();
         it != timers_.end(); ++it)
    {
        delete it->second;
    }
}

void TimerManager::handleRead()
{
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);

    std::vector<Entry> expired = getExpired(now);
    callingExpiredTimers_ = true;
    for (auto it = expired.begin(); it != expired.end(); ++it)
    {
        //��ʱ�ص�����
        it->second->run();
    }
    callingExpiredTimers_ = false;
    reset(expired, now);
}

void TimerManager::addTimer(const TimerCallback& cb,
                            Timestamp when,
                            double interval)
{
    Timer* timer = new Timer(cb, when, interval);
    loop_->runInLoop(
            boost::bind(&TimerManager::addTimerInLoop, this, timer));
}

void TimerManager::addTimerInLoop(Timer* timer)
{
    loop_->assertInLoopThread();
    //����һ����ʱ�����п��ܻ�ı����ȶ������絽��ʱ��
    bool earlistChanged = insert(timer);
    if (earlistChanged)
    {
        resetTimerfd(timerfd_, timer->expiration());
    }
}

//ִ�е��ڵĶ�ʱ������Ҫ���ظ�ִ�еĶ�ʱ����������С������
void TimerManager::reset(const std::vector<Entry>& expired, Timestamp now)
{
    Timestamp nextExpired;
    for (std::vector<Entry>::const_iterator it = expired.begin();
         it != expired.end(); ++it) {
        //canceling_
        if (it->second->repeat()) {
            it->second->restart(now);
            insert(it->second);
        } else {
            delete it->second;
        }
    }
    
    if (!timers_.empty())
    {
        nextExpired = timers_.begin()->second->expiration();
    }

    if (nextExpired.valid())
    {
        resetTimerfd(timerfd_, nextExpired);
    }
}

bool TimerManager::insert(Timer* timer)
{
    loop_->assertInLoopThread();
    bool earlistChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if (it == timers_.end() || when < it->first)
    {
        earlistChanged = true;
    }
    //���뵽timers_��
    timers_.insert(Entry(when, timer));
    return earlistChanged;
}

std::vector<TimerManager::Entry> TimerManager::getExpired(Timestamp now)
{
    std::vector<Entry> expired;
    for (auto it = timers_.begin(); it != timers_.end();) {
        if (it->first <= now) 
        {
            expired.push_back(*it);
            it = timers_.erase(it);
        }
        else
        {
            break;
        }
    }
    return expired;
}

}//namspace net
}//namespace ywl
