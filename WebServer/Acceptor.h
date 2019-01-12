#ifndef WEBSERVER_ACCEPTOR_H
#define WEBSERVER_ACCEPTOR_H

#include <WebServer/Channel.h>
#include <WebServer/Socket.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

namespace ywl
{
namespace net
{
class EventLoop;
class InetAddress;

class Acceptor : boost::noncopyable
{
public:
    typedef boost::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;
    Acceptor(EventLoop* loop, const InetAddress& listenAddr);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    {
        NewConnectionCallback_ = cb;
    }
    bool listenning() const {  return listenning_; }
    void listen();
private:
    void handleRead();

    EventLoop* loop_;
    //由acceptSocket_负责关闭监听描述符
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback NewConnectionCallback_;
    bool listenning_;
    int idleFd_;
};

}
}

#endif
