#ifndef WEB_SERVER_EVENTLOOP_H
#define WEB_SERVER_EVENTLOOP_H

#include "base/Mutex.h"
#include "base/Thread.h"
#include "base/Timestamp.h"
#include "Channel.h"
#include "TimerId.h"

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>

namespace ywl
{
namespace net
{

class Channel;
class Poller;

class EventLoop : boost::noncopyable
{
public:
    using Functor = boost::function<void()>;
    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }
    void runInLoop(const Functor& cb);
    void queueInLoop(const Functor& cb);

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {
            abortNotInLoopThread();
        }
    }
    bool isInLoopThread() const
    {
        return threadId_ == CurrentThread::tid();
    }
    static EventLoop* getEventLoopOfCurrentThread();

private:
    void abortNotInLoopThread();

    typedef std::vector<Channel*> ChannelList;

    bool quit_;
    bool looping_;
    bool eventHandling_;
    const pid_t threadId_;

    Timestamp pollReturnTime_;
    boost::scoped_ptr<Poller> poller_;

    ChannelList activeChannels_;
    Channel* currentActiveChannel_;
};

}
}
#endif
