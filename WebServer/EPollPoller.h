#ifndef WEB_SERVER_EPOLLPOLLER_H
#define WEB_SERVER_EPOLLPOLLER_H

#include "Poller.h"

#include <vector>
#include <map>

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
    enum class Operation
    {
        ADD = 1,    //EPOLL_CTL_ADD
        DEL = 2,    //EPOLL_CTL_DEL
        MOD = 3     //EPOLL_CTL_MOD
    };
    static const int kInitEventListSize = 16;   //返回事件初始化大小，以二倍增长速度扩容

    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(Operation ope, Channel* channel);

    using EventList = std::vector<struct epoll_event>;
    using ChannelMap = std::map<int, Channel*>;

    int epollfd_;
    EventList events_;
    ChannelMap channels_;
};

}
}
#endif
