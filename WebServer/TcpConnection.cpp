#include <WebServer/TcpConnection.h>
#include <WebServer/base/Logging.h>
#include <WebServer/Channel.h>
#include <WebServer/Util.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <stdio.h>

using namespace ywl;
using namespace ywl::net;

TcpConnection::TcpConnection(EventLoop* loop,
                                  const std::string& name,
                                  int sockfd,
                                  const InetAddress& localAddr,
                                  const InetAddress& peerAddr)
    : loop_(loop),
      state_(Connecting),
      name_(name),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64*1024*1024)
{
    channel_->setReadCallback(
        boost::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallback(
        boost::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        boost::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        boost::bind(&TcpConnection::handleError, this));
    LOG << "TcpConnection::ctor[" << name_ << "] at " << this
        << " fd=" << sockfd;
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG << "TcpConnection::dtor[" << name_ << "] at " << this
        << " fd=" << channel_->fd();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    loop_->assertInLoopThread();
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0) {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG << "SYSERR-TcpConnection::hanleRead";
        handleError();
    }
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG << "fd= " << channel_->fd() << " state= " << state_;
    assert(state_ == Connected || state_ == Disconnecting);
    setState(Disconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    // connectionCallback_(guardThis);
    
    closeCallback_(guardThis);
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    LOG << "TcpConnection::handleError [" << name_ 
        << "] -SO_ERROR = " << err;
}
