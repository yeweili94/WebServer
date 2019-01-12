#include <WebServer/EPollPoller.h>
#include <WebServer/Channel.h>
#include <WebServer/base/Logging.h>

#include <boost/static_assert.hpp>
#include <boost/implicit_cast.hpp>

#include <poll.h>
#include <sys/epoll.h>
#include <assert.h>
#include <errno.h>

using namespace ywl;
using namespace ywl::net;
using boost::implicit_cast;

BOOST_STATIC_ASSERT(EPOLLIN == POLLIN);
BOOST_STATIC_ASSERT(EPOLLPRI == POLLPRI);
BOOST_STATIC_ASSERT(EPOLLOUT == POLLOUT);
BOOST_STATIC_ASSERT(EPOLLRDHUP == POLLRDHUP);
BOOST_STATIC_ASSERT(EPOLLERR == POLLERR);
BOOST_STATIC_ASSERT(EPOLLHUP == POLLHUP);

namespace
{
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}

EPollPoller::EPollPoller(EventLoop* loop)
  : Poller(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG << "EPollPoller::EPollPoller";
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    int numEvents = ::epoll_wait(epollfd_,
                                 &*events_.begin(),
                                 static_cast<int>(events_.size()),
                                 timeoutMs);
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOG << numEvents << " events happended";
        fillActiveChannels(numEvents, activeChannels);
        if (boost::implicit_cast<size_t>(numEvents) == events_.size())
        {
            events_.resize(events_.size()*2);
        }
    }
    else if (numEvents == 0)
    {
        LOG << "nothing happended";
    }
    else
    {
        LOG << "EPollPoller::poll()";
    }
    return now;
}

void EPollPoller::fillActiveChannels(int numEvents,
                                     ChannelList* activeChannels) const
{
    assert(implicit_cast<size_t>(numEvents) <= events_.size());
    for (int i = 0; i < numEvents; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
#ifdef DEBUG
        int fd = channel->fd();
        ChannelMap::const_iterator it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
#endif
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
  }
}

void EPollPoller::updateChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    LOG << "fd = " << channel->fd() << " events = " << channel->events();
    const int status = channel->status();
    if (status == kNew || status == kDeleted)
    {
        // a new one, add with EPOLL_CTL_ADD
        int fd = channel->fd();
        if (status == kNew)
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        else // status == kDeleted
        {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->set_status(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        (void)fd;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(status == kAdded);
        if (channel->isNoneEvent())
        {
            //这里只在内核中关闭对该事件的关注
            //并不清除map中该channel的记录
            update(EPOLL_CTL_DEL, channel);
            channel->set_status(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    int fd = channel->fd();
    LOG << "fd = " << fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());
    int status = channel->status();
    assert(status == kAdded || status == kDeleted);
    size_t n = channels_.erase(fd);
    (void)n;
    assert(n == 1);

    if (status == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_status(kNew);
}

void EPollPoller::update(int operation, Channel* channel)
{
    struct epoll_event event;
    bzero(&event, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG << "epoll_ctl op=" << operation << " fd=" << fd;
        }
        else
        {
            LOG << "epoll_ctl op=" << operation << " fd=" << fd;
        }
    }
}
