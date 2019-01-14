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
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
  }
}

void EPollPoller::updateChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    LOG << "fd = " << channel->fd() << " events = " << channel->events();
    const int status = channel->status();
    if (status == kNew)
    {
        channel->set_status(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
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
    assert(channel->isNoneEvent());
    int status = channel->status();
    assert(status == kAdded);
    (void)status;
    update(EPOLL_CTL_DEL, channel);
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
