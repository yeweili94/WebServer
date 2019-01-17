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

//这里需要提供参数timestamp是为了方便加入时间管理
class EventLoop;
class Channel : boost::noncopyable
{
 public:
  typedef boost::function<void()> EventCallback;
  typedef boost::function<void(Timestamp)> ReadEventCallback;

  //channel类虽然是和文件描述符fd相互绑定的类,但是
  //却并不负责关闭fd，原因是这个fd,是在构造时候从外部传进来的
  //谁污染，谁治理，RAII封装
  Channel(EventLoop* loop, int fd);
  ~Channel();

  void handleEvent(Timestamp receiveTime);
  void setReadCallback(const ReadEventCallback& cb)
  { readCallback_ = cb; }
  void setWriteCallback(const EventCallback& cb)
  { writeCallback_ = cb; }
  void setCloseCallback(const EventCallback& cb)
  { closeCallback_ = cb; }
  void setErrorCallback(const EventCallback& cb)
  { errorCallback_ = cb; }

  int fd() const { return fd_; }
  int events() const { return events_; }
  void set_revents(int revt) { revents_ = revt; } // used by pollers
  int revents() const { return revents_; }
  bool isNoneEvent() const { return events_ == kNoneEvent; }

  void enableReading() { events_ |= kReadEvent; update(); }
  void disableReading() { events_ &= ~kReadEvent; update(); }
  void enableWriting() { events_ |= kWriteEvent; update(); }
  void disableWriting() { events_ &= ~kWriteEvent; update(); }
  void disableAll() { events_ = kNoneEvent; update(); }
  bool isWriting() const { return events_ & kWriteEvent; }

  // for Poller
  int status() { return status_; }
  void setStatus(int status) { status_ = status; }

  EventLoop* ownerLoop() { return loop_; }
  void remove();

 private:
  void update();

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop* loop_;
  const int  fd_;
  int        events_;
  int        revents_;
  int        status_; 

  bool eventHandling_;
  ReadEventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};

}
}

#endif

