#ifndef WEB_SERVER_EPOLLPOLLER_H
#define WEB_SERVER_EPOLLPOLLER_H

#include <vector>
#include <map>

struct epoll_event;

namespace ywl
{
namespace net
{

class EpollPoller
{
public:
    EpollPoller(EventLoop* loop);
    virtual ~EpollPoller();
private:
};

}//namespace net
}//namespace ywl
#endif
