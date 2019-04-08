#ifndef WEB_SERVER_NET_TIMER_H
#define WEB_SERVER_NET_TIMER_H

#include <WebServer/base/Mutex.h> 
#include <WebServer/base/Timestamp.h>
#include <WebServer/Channel.h>

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>

#include <set>
#include <queue>
#include <atomic>
#include <sys/select.h>

namespace ywl
{
namespace net
{
typedef boost::function<void()> TimerCallback;

class Timer : boost::noncopyable
{
public:
    Timer(const TimerCallback& cb, Timestamp when, double interval)
        : callback_(cb),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0)
    {
        Timer::timerId_.fetch_add(0);
    }
    
    void run() {
        if (callback_) callback_();
    }

    ~Timer() {
        run();
    }

    Timestamp expiration() const { return expiration_; }
    int64_t TimerId() { return Timer::timerId_.fetch_add(0); }
    int msecSecondsSinceEpoch() { return expiration_.msecSecondsSinceEpoch(); }
    bool repeat() const { return repeat_; }
    bool isValid() const { return expiration_.valid(); }
    void restart(Timestamp now)
    {
        if (repeat_) {
            expiration_ = addTime(now, interval_);
        } else {
            expiration_ = Timestamp::invalid();
        }
    }
private:
    const TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
    static std::atomic<uint64_t> timerId_;
};

struct TimerCmp
{
    bool operator() (const std::shared_ptr<Timer>& lt, const std::shared_ptr<Timer>& rt)
    {
        if (lt->msecSecondsSinceEpoch() > rt->msecSecondsSinceEpoch()) return true;
        if (lt->msecSecondsSinceEpoch() == rt->msecSecondsSinceEpoch() && lt->TimerId() > rt->TimerId()) return true;
        return false;
    }
};

class EventLoop;
class TimerManager : public boost::noncopyable
{
public:
    TimerManager(EventLoop* loop);
    ~TimerManager();
    void addTimer(const TimerCallback& cb, Timestamp when, double interval);
    void handleExpired();
    bool earliestChanged(const std::shared_ptr<Timer>& timerPtr);
    int nextExpired();

private:
    EventLoop* loop_;
    typedef std::shared_ptr<Timer> TimerPtr;
    std::priority_queue<TimerPtr, std::vector<TimerPtr>, TimerCmp> timerQueue_;
};

}
}

#endif
