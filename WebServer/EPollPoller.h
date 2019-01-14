#ifndef WEB_SERVER_EPOLLPOLLER_H
#define WEB_SERVER_EPOLLPOLLER_H

#include <WebServer/Poller.h>

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
    static const int kInitEventListSize = 16;   //返回事件初始化大小，以二倍增长速度扩容

    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(int ope, Channel* channel);

    using EventList = std::vector<struct epoll_event>;

    int epollfd_;
    EventList events_;
};

}
}
#endif
