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
    void handleRead();  //�¼�����ʱ�Ļص�����

    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);
    bool insert(Timer* timer);

    EventLoop* loop_;   //������EventLoop
    const int timerfd_; //����ʱд����ļ�������
    Channel timerfdChannel_;    
    TimerList timers_;  //����timestamp����
    bool callingExpiredTimers_; 
};

}
}

#endif
