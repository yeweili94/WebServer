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

#include <sys/epoll.h>

namespace ywl
{
namespace net
{

class Channel;
class Poller;
class TimerManager;

class EventLoop : boost::noncopyable
{
public:
    typedef boost::function<void()> Functor;

    EventLoop();
    ~EventLoop();  

    void loop();
    void quit();

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
    void updateChannel(Channel* channel);		// ��ePoller����ӻ��߸���ͨ��
    void removeChannel(Channel* channel);		// ��ePoller���Ƴ�ͨ��

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
    bool looping_; 
    bool quit_; 
    bool eventHandling_; 
    bool callingPendingFunctors_; 
    const pid_t threadId_;		
    //EPOLL
    int epollfd_;
    std::vector<struct epoll_event> epollEvents_;

    boost::scoped_ptr<TimerManager> timerQueue_;
    int wakeupFd_;				// ����eventfd
    boost::scoped_ptr<Channel> wakeupChannel_;	// ��ͨ����������poller_������

    std::vector<Channel*> activeChannels_;		// Poller���صĻͨ��
    Channel* currentActiveChannel_;	// ��ǰ���ڴ���Ļͨ��

    MutexLock mutex_;
    std::vector<Functor> pendingFunctors_; // GuardedBy mutex_

    Timestamp pollReturnTime_;
    Timestamp poll(int timeoutMs);
    void fillActiveChannels(int numEvents);
    void epollUpdateChannel(Channel* channel);
    void epollRemoveChannel(Channel* channel);
    void epollUpdate(int operation, Channel* channel);
};

}
}
#endif
