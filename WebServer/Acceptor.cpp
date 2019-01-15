#include <WebServer/EventLoop.h>
#include <WebServer/Acceptor.h>
#include <WebServer/Util.h>
#include <netinet/in.h>
#include <WebServer/InetAddress.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <fcntl.h>

using namespace ywl;
using namespace ywl::net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
    : loop_(loop),
      acceptFd_(sockets::createNonBlockSocketfd()),
      acceptChannel_(loop, acceptFd_),
      listenning_(false),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    assert(idleFd_ >= 0);
    sockets::SetReuseAddr(acceptFd_, true);
    sockets::Bind(acceptFd_, listenAddr.getSockAddrInet());
    acceptChannel_.setReadCallback(
        boost::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listenning_ = true;
    sockets::Listen(acceptFd_);
    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(0);
    struct sockaddr_in addr;
    bzero(&addr, sizeof addr);
    int connfd = sockets::Accept(acceptFd_, &addr);
    peerAddr.setSockAddrInet(addr);

    if (connfd >= 0)
    {
        if (NewConnectionCallback_)
        {
            NewConnectionCallback_(connfd, peerAddr);
        }
        else
        {
            sockets::Close(connfd);
        }
    }
    else
    {
        if (errno == EMFILE)
        {
            ::close(idleFd_);
            idleFd_ = ::accept(acceptFd_, NULL, NULL);
            ::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}
