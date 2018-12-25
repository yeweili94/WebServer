#ifndef WEB_SERVER_POLLER_H
#define WEB_SERVER_POLLER_H

#include "EventLoop.h"
#include "base/Timestamp.h"

#include <boost/noncopyable.hpp>

#include <vector>
#include <map>

namespace ywl
{
namespace net
{

class Channel;

class Poller
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop) {ownerLoop_ = loop;}
    virtual ~Poller() {}

    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    static Poller* newDefaultPoller(EventLoop* loop);

    void assertInLoopThread()
    {
        ownerLoop_->assertInLoopThread();
    }
private:
    EventLoop* ownerLoop_;
};

}//namespace net
}//namespace ywl

#endif