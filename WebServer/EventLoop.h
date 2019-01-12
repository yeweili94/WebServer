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
    void updateChannel(Channel* channel);		// ��Poller����ӻ��߸���ͨ��
    void removeChannel(Channel* channel);		// ��Poller���Ƴ�ͨ��

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
    const pid_t threadId_;		// ��ǰ���������߳�ID
    Timestamp pollReturnTime_;
    boost::scoped_ptr<Poller> poller_;
    boost::scoped_ptr<TimerManager> timerQueue_;
    int wakeupFd_;				// ����eventfd
    boost::scoped_ptr<Channel> wakeupChannel_;	// ��ͨ����������poller_������
    ChannelList activeChannels_;		// Poller���صĻͨ��
    Channel* currentActiveChannel_;	// ��ǰ���ڴ���Ļͨ��
    MutexLock mutex_;
    std::vector<Functor> pendingFunctors_; // @BuardedBy mutex_
};

}
}
#endif
