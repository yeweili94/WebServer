#ifndef WEB_SERVER_EPOLLPOLLER_H
#define WEB_SERVER_EPOLLPOLLER_H

#include "Poller.h"

#include <vector>
#include <unordered_map>

struct epoll_event;

namespace ywl
{
namespace net
{

class EPollPoller : public Poller {
public:
    EPollPoller(EventLoop* loop);
    virtual ~EPollPoller();

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    enum Operation
    {
        ADD = 1,
        DEL = 2,
        MOD = 3
    };
    static const int kInitEventListSize = 16;

    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(Operation ope, Channel* channel);

    using EventList = std::vector<struct epoll_event>;
    using ChannelMap = std::unordered_map<int, Channel*>;

    int epollfd_;
    EventList events_;
    ChannelMap channels_;
};

}
}
#endif
