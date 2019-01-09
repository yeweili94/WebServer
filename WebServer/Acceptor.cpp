#include <WebServer/EventLoop.h>
#include <WebServer/Acceptor.h>
#include <WebServer/Util.h>
#include <WebServer/InetAddress.h>
#include <WebServer/Socket.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <fcntl.h>

using namespace ywl;
using namespace ywl::net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
    : loop_(loop),
      acceptSocket_(sockets::createNonBlocking()),
      acceptChannel_(loop, acceptSocket_.fd()),
      listenning_(false),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    assert(idleFd_ >= 0);
}
