#include <WebServer/Channel.h>
#include <WebServer/EventLoop.h>
#include <WebServer/base/Logging.h>
#include <WebServer/base/Singleton.h>

#include <sstream>
#include <poll.h>

using namespace ywl;
using namespace ywl::net;

static MemoryPool<sizeof(Channel)> CHANNEL_MEMORY_POOL;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      status_(-1)  //KNew
{
    assert(status_ == -1);
}

Channel::~Channel()
{
}

void* Channel::operator new(size_t size) {
    (void)size;
    return CHANNEL_MEMORY_POOL.malloc();
}

void Channel::operator delete(void* p) {
    CHANNEL_MEMORY_POOL.free(p);
}

void Channel::update()
{
    loop_->updateChannel(this);
}

//remove之前确保调用了disableAll
void Channel::remove()
{
    assert(isNoneEvent());
    loop_->removeChannel(this);
}

//事件分发器
void Channel::handleEvent(Timestamp receiveTime)
{
    //POLLIN 普通可读事件
    //POLLPRI 高优先级事件(如带外数据)
    //POLLOUT 可读事件
    //POLLERR 发生错误
    //POLLHUP　对方描述符挂起
    //POLLNVAL 描述字不是一个打开的文件
    //这里关于POLLHUP什么时候会触发？
    //正常情况下如果是客户端关闭连接，则只会触发POLLIN,
    //若是服务器端主动关闭连接，导致客户端再关闭连接，则会触发POLLIN 和 POLLHUP
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        LOG << "Channel::handle_event() POLLHUP";
        if (closeCallback_) closeCallback_();
    }
    if (revents_ & POLLNVAL)
    {
        LOG << "Channel::handle_event() POLLNVAL";
    }
    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (errorCallback_) errorCallback_();
    }
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
    {
        if (readCallback_) readCallback_(receiveTime);
    }
    if (revents_ & POLLOUT)
    {
        if (writeCallback_) writeCallback_();
    }
}

