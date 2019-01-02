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


//�����൱��һ��pair��
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
    //TimerList������ʱ������ActiveTimerSet��Timer�����ַ����
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::pair<Timer*, int64_t> ActiveTimer;
    typedef std::set<Entry> TimerList;
    typedef std::set<ActiveTimer> ActiveTimerSet;

    void addTimerInLoop(Timer* timer);
    void cancelInLoop(TimerId timerId);
    void handleRead();  //�¼�����ʱ�Ļص�����

    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);
    bool insert(Timer* timer);

    EventLoop* loop_;   //������EventLoop
    const int timerfd_; //����ʱд����ļ�������
    Channel timerfdChannel_;    
    TimerList timers_;  //����timestamp����
    ActiveTimerSet activeTimers_;   //���ն����ַ����
    bool callingExpiredTimers_; 
    ActiveTimerSet cancelingTimers_;    //ȡ���Ķ�ʱ��
};

}
}
#endif
