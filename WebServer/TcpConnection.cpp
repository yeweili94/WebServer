#include <WebServer/TcpConnection.h>
#include <WebServer/base/Logging.h>
#include <WebServer/Channel.h>
#include <WebServer/Util.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <stdio.h>

namespace ywl
{
namespace net
{
void defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    LOG << conn->localAddress().toIpPort() << " -> "
        << conn->peerAddress().toIpPort() << " is "
        << (conn->connected() ? "UP" : "DOWN");
}

void defaultMessageCallback(const TcpConnectionPtr&, Buffer* buf, Timestamp)
{
    std::string message = buf->retrieveAllAsString();
    fprintf(stderr, "%s\n", message.c_str());
}

}
}

using namespace ywl;
using namespace ywl::net;

//在TcpServer中的Acceptor中设置的NewConnectionCallback_，sockfd
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
    //channel产生可读事件，回调handleRead()函数
    channel_->setReadCallback(
        boost::bind(&TcpConnection::handleRead, this, _1));
    //channel产生可写事件，回调handleWrite()函数
    channel_->setWriteCallback(
        boost::bind(&TcpConnection::handleWrite, this));
    //连接关闭，回调haneleClose()函数
    channel_->setCloseCallback(
        boost::bind(&TcpConnection::handleClose, this));
    //发生错误，回调handleError()函数
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

void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if (channel_->isWriting())
    {
        ssize_t n = sockets::Write(channel_->fd(),
                                   outputBuffer_.peek(),
                                   outputBuffer_.readableBytes());
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting(); //停止关注可写事件,避免出现busy loop
                if (writeCompleteCallback_)
                {
                    loop_->runInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == Disconnecting) //所有的数据都发送完毕，并且有人试图关闭连接，则可以关闭连接
                {                            //这个是shutdownInLoop是因为在shutdown()函数中有可能关闭不成功
                    shutdownInLoop();        //需要在发送缓冲区发送完毕时再做一次判断
                }
            }
            else
            {
                LOG << "data has not been write complete";
            }
        }
        else
        {
            LOG << "TcpConnection::handleWrite()";
        }
    }
    else
    {
        LOG << "Connection fd = " << channel_->fd()
            << " is down, can not writting";
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

//当应用层想要关闭连接，不能直接调用handleclose
//因为有可能outputBuffer中的数据还没有发送完
//也就是说conn->send()只要网络没有故障就应该保证完整的发送所有消息
void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    //如果还在关注POLLOUT事件，也就是说还有数据没有发送完,这时候我们不能关闭连接
    //此时的shutdownWrite应该在handleWrite中数据发送完毕后再调用一次这个函数
    //此时判断的依据是tcpconnection的状态,是否为Disconnecting
    if (!channel_->isWriting())
    {
        socket_->shutdownWrite();
    }
}

void TcpConnection::shutdown()
{
    if (state_ == Connected)
    {
        //将状态改为Disconnecting
        setState(Disconnecting);
        loop_->runInLoop(boost::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t nleft = len;
    bool error = false;
    if (state_ == Disconnected)
    {
        LOG << "WARN: disconnected, give up writting";
        return;
    }
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = sockets::Write(channel_->fd(), data, len);
        if (nwrote >= 0) {
            nleft = len - nwrote;
            if (nleft ==0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG << "SYSERR: Tcpconnection::sendInLoop";
                if (errno  == EPIPE)
                {
                    error = true;
                }
            }
        }
    }
    assert(nleft <= len);
    if (!error && nleft)
    {
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + oldLen >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMarkCallback_)
        {
            loop_->queueInLoop(boost::bind(highWaterMarkCallback_, shared_from_this(), oldLen + nleft));
        }
        outputBuffer_.append(static_cast<const char*>(data)+nwrote, nleft);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::sendInLoop(const std::string& message)
{
    sendInLoop(message.c_str(), message.size());
}

void TcpConnection::send(const void* data, size_t len)
{
    if (state_ == Connected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(data, len);
        }//若是其他线程调用的则把message拷贝一份
        else
        {
            std::string message(static_cast<const char*>(data), len);
            loop_->runInLoop(
                boost::bind(&TcpConnection::sendInLoop, this, message));
        }
    }
}

void TcpConnection::send(const std::string& message)
{
    if (state_ == Connected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message);
        }
        else
        {
            loop_->runInLoop(boost::bind(&TcpConnection::sendInLoop, this, message));
        }
    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
    socket_->setTcpNoDelay(on);
}

void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == Connecting);
    setState(Connected);

    channel_->tie(shared_from_this());
    channel_->enableReading();
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    if (state_ == Connected)
    {
        setState(Disconnecting);
        channel_->disableAll();
        if (connectionCallback_) {
            connectionCallback_(shared_from_this());
        }
    }
    channel_->remove();
}

