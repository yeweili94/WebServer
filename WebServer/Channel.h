#ifndef WEB_SERVER_CHANNEL_H
#define WEB_SERVER_CHANNEL_H

#include <WebServer/base/Timestamp.h>

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/any.hpp>

namespace ywl
{
namespace net
{

using EventCallback = boost::function<void()>;
using ReadEventCallback = boost::function<void(Timestamp)>;
class EventLoop;

class Channel : public boost::noncopyable
{
public:
    explicit Channel(EventLoop* loop, int fd);
    ~Channel();

    //epoll_wait返回的时间
    void handleEvent(Timestamp receiveTime);
    void setReadCallback(const ReadEventCallback& cb)
    {
        readCallback_ = cb;
    }
    void setWriteCallback(const EventCallback& cb)
    {
        writeCallback_ = cb;
    }
    void setCloseCallback(const EventCallback& cb)
    {
        closeCallback_ = cb;
    }
    void setErrorCallback(const EventCallback& cb)
    {
        errorCallback_ = cb;
    }
    void enableReading() { events_ |= kReadEvent; update();}
    void disableReading() { events_ &= ~kWriteEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    void tie(const boost::shared_ptr<boost::any>& any);
    int fd() const { return fd_; }

    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }

    int index() { return index_; }
    int events() const { return events_; }
    void set_index(int idx) { index_ = idx; }
    void set_revents(int revt) {revents_ = revt; }
    void printRevent() const;

    void doNotLogHup() { logHup_ = false; }

    EventLoop* ownerLoop() { return loop_; }
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;
    const int fd_;  //文件描述符
    int events_;    //关注的事件
    int revents_;   //在epoll中返回的事件
    int index_;     //在epoll中表示Channel中的状态(added\removed\new)
    bool logHup_;
    bool eventHandling_;

    boost::weak_ptr<boost::any> tie_;
    bool tied_;

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};

}
}

#endif

