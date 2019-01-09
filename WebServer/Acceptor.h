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
};

}
}

#endif
