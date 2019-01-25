#ifndef WEB_SERVER_NET_TIMER_H
#define WEB_SERVER_NET_TIMER_H

#include <WebServer/base/Mutex.h> 
#include <WebServer/base/Timestamp.h>
#include <WebServer/Channel.h>

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>

#include <set>

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
          repeat_(interval > 0),
          sequence_(s_numCreated_.incrementAndGet())
    {}

    void run() const
    {
        callback_();
    }

    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }

    void restart(Timestamp now)
    {
        if (repeat_) {
            expiration_ = addTime(now, interval_);
        } else {
            expiration_ = Timestamp::invalid();
        }
    }

    static int64_t numCreated() { return s_numCreated_.get(); }

private:
    const TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
    const int64_t sequence_;

    static AtomicInt64 s_numCreated_;
};


//这里为什么要维护这样一个结构？
//因为我们有可能会取消某一个定时器，而这个定时器的timestamp是可变的
//因此map的key不能是timestamp，我们需要维护一个以timestamp排序的堆
//和一个以Timer*排序的堆，由于每一个定时器都有唯一的编号，所以我们
//用一个TimerId包裹一下这个定时器，以便于删除定时器
class TimerId 
{
public:
    TimerId() : timer_(NULL),
                sequence_(0)
    {
    }
    TimerId(Timer* timer, int64_t sequence)
        : timer_(timer),
          sequence_(sequence)
    {
    }
    ~TimerId() = default;
    friend class TimerManager;
private:
    Timer* timer_;
    int64_t sequence_;
};


class EventLoop;
class TimerManager : public boost::noncopyable
{
public:
    TimerManager(EventLoop* loop);
    ~TimerManager();

    //thread safe, it's can called from other threads
    TimerId addTimer(const TimerCallback& cb, Timestamp when, double interval);
    void cancel(TimerId timerId);
private:
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::pair<Timer*, int64_t> ActiveTimer;

    //TimerList按到期时间排序，ActiveTimerSet按Timer对象地址排序
    typedef std::set<Entry> TimerList;
    typedef std::set<ActiveTimer> ActiveTimerSet;

    void addTimerInLoop(Timer* timer);
    void cancelInLoop(TimerId timerId);
    void handleRead();  //事件到达时的回调函数

    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);
    bool insert(Timer* timer);

    EventLoop* loop_;   //所属的EventLoop
    const int timerfd_; //唤醒时写入的文件描述符
    Channel timerfdChannel_;    

    TimerList timers_;  //按照timestamp排序

    //活着的定时器,以便于cancel使用
    ActiveTimerSet activeTimers_;   //按照对象地址排序
    ActiveTimerSet cancelingTimers_;    //取消的定时器
    bool callingExpiredTimers_; 
};

}
}

#endif
