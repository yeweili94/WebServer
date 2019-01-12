#ifndef WEB_SERVER_EVENTLOOP_H
#define WEB_SERVER_EVENTLOOP_H

#include <WebServer/base/Mutex.h>
#include <WebServer/base/Thread.h>
#include <WebServer/base/Timestamp.h>
#include <WebServer/Timer.h>
#include <WebServer/Channel.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>

namespace ywl
{
namespace net
{

class Channel;
class Poller;
class TimerManager;
///
/// Reactor, at most one per thread.
///
/// This is an interface class, so don't expose too much details.
class EventLoop : boost::noncopyable
{
public:
    typedef boost::function<void()> Functor;

    EventLoop();
    ~EventLoop();  // force out-line dtor, for scoped_ptr members.

    ///
    /// Must be called in the same thread as creation of the object.
    ///
    void loop();

    void quit();

    ///
    /// Time when poll returns, usually means data arrivial.
    ///
    Timestamp pollReturnTime() const { return pollReturnTime_; }

    /// Safe to call from other threads.
    void runInLoop(const Functor& cb);
    void queueInLoop(const Functor& cb);

    // timers
    // Safe to call from other threads.
    TimerId runAt(const Timestamp& time, const TimerCallback& cb);
    TimerId runAfter(double delay, const TimerCallback& cb);
    TimerId runEvery(double interval, const TimerCallback& cb);
    void cancel(TimerId timerId);

    // internal usage
    void wakeup();
    void updateChannel(Channel* channel);		// 在Poller中添加或者更新通道
    void removeChannel(Channel* channel);		// 从Poller中移除通道

    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abortNotInLoopThread();
        }
    }
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    bool eventHandling() const { return eventHandling_; }

    static EventLoop* getEventLoopOfCurrentThread();

private:
    void abortNotInLoopThread();
    void handleRead();  // waked up
    void doPendingFunctors();
    typedef std::vector<Channel*> ChannelList;
    bool looping_; /* atomic */
    bool quit_; /* atomic */
    bool eventHandling_; /* atomic */
    bool callingPendingFunctors_; /* atomic */
    const pid_t threadId_;		// 当前对象所属线程ID
    Timestamp pollReturnTime_;
    boost::scoped_ptr<Poller> poller_;
    boost::scoped_ptr<TimerManager> timerQueue_;
    int wakeupFd_;				// 用于eventfd
    boost::scoped_ptr<Channel> wakeupChannel_;	// 该通道将会纳入poller_来管理
    ChannelList activeChannels_;		// Poller返回的活动通道
    Channel* currentActiveChannel_;	// 当前正在处理的活动通道
    MutexLock mutex_;
    std::vector<Functor> pendingFunctors_; // @BuardedBy mutex_
};

}
}
#endif
