#include <WebServer/EPollPoller.h>
#include <WebServer/Channel.h>
#include <WebServer/base/Logging.h>

#include <boost/static_assert.hpp>

#include <poll.h>
#include <sys/epoll.h>
#include <assert.h>
#include <errno.h>

using namespace ywl;
using namespace ywl::net;

BOOST_STATIC_ASSERT(EPOLLIN == POLLIN);
BOOST_STATIC_ASSERT(EPOLLPRI == POLLPRI);
BOOST_STATIC_ASSERT(EPOLLOUT == POLLOUT);
BOOST_STATIC_ASSERT(EPOLLERR == POLLERR);
BOOST_STATIC_ASSERT(EPOLLHUP == POLLHUP);
BOOST_STATIC_ASSERT(EPOLLRDHUP == POLLRDHUP);

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
        FATAL << "EPollPoller::EPollPoller";
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activateChannels)
{
    int numEvents = ::epoll_wait(epollfd_,
                                 &*events_.begin(),
                                 static_cast<int>(events_.size()),
                                 timeoutMs);
    Timestamp now(Timestamp::now());
    if (numEvents > 0) 
    {
        LOG << numEvents << " events happended";
        fillActiveChannels(numEvents, activateChannels);
        if (static_cast<size_t>(numEvents) == events_.size())
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
        FATAL << " SYSERR-EpollPoller::poll()";
    }
    return now;
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    assert(static_cast<size_t>(numEvents) <= events_.size());
    for (int i = 0; i < numEvents; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        //debug
        int fd = channel->fd();
        ChannelMap::const_iterator it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
        it = channels_.end();

        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void EPollPoller::updateChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    LOG << "EPollPoller::updateChannel() fd = " << channel->fd() \
        << " events = " << channel->events();
    const int index = channel->index();
    //add new one
    if (index == kNew || index == kDeleted)
    {
        int fd = channel->fd();
        if (index == kNew)
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        else //index == kDeleted
        {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->set_index(kAdded);
        update(Operation::ADD, channel);
    }
    else
    {
        //update existing one with EPOLL_CTL or MOD/DEL 
        int fd = channel->fd(); (void)fd;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);
        if (channel->isNoneEvent())
        {
            update(Operation::DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(Operation::MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    int fd = channel->fd();
    LOG << " fd = " << fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());

    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    size_t n = channels_.erase(fd);
    (void)n;
    assert(n == 1);
    if (index == kAdded)
    {
        update(Operation::DEL, channel);
    }
    channel->set_index(kNew);
}

void EPollPoller::update(Operation ope, Channel* channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if (::epoll_ctl(epollfd_, static_cast<int>(ope), fd, &event) < 0)
    {
        if (static_cast<int>(ope) == EPOLL_CTL_DEL)
        {
            LOG<< "SYSERR: epoll_ctl op = " << static_cast<int>(ope) << " fd = " << fd;
        }
        else
        {
            FATAL << "epoll_ctl op = " << static_cast<int>(ope) << " fd = " << fd;
        }
    }
}

