#ifndef WEB_SERVER_NET_TIMER_H
#define WEB_SERVER_NET_TIMER_H

#include <WebServer/base/Mutex.h> 
#include <WebServer/base/Timestamp.h>
#include <WebServer/Channel.h>

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>

#include <set>
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
    {}

    void run() const
    {
        callback_();
    }

    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
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
};

class EventLoop;
class TimerManager : public boost::noncopyable
{
public:
    TimerManager(EventLoop* loop);
    ~TimerManager();
    void addTimer(const TimerCallback& cb, Timestamp when, double interval);

private:
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::set<Entry> TimerList;

    void addTimerInLoop(Timer* timer);
    void handleRead();  //事件到达时的回调函数

    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);
    bool insert(Timer* timer);

    EventLoop* loop_;   //所属的EventLoop
    const int timerfd_; //唤醒时写入的文件描述符
    Channel timerfdChannel_;    
    TimerList timers_;  //按照timestamp排序
    bool callingExpiredTimers_; 
};

}
}

#endif
