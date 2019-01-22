#include <WebServer/TcpConnection.h>
#include <WebServer/base/Logging.h>
#include <WebServer/base/Singleton.h>
#include <WebServer/Channel.h>
#include <WebServer/Util.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <stdio.h>

using namespace ywl;
using namespace ywl::net;

static MemoryPool<sizeof(TcpConnection)> TCPCONN_MEMORY_POOL_;
//在TcpServer中的Acceptor中设置的NewConnectionCallback_，sockfd
TcpConnection::TcpConnection(EventLoop* loop,
                                  const std::string& name,
                                  int sockfd,
                                  const InetAddress& localAddr,
                                  const InetAddress& peerAddr)
    : loop_(loop),
      state_(Connecting),
      name_(name),
      sockfd_(sockfd),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64*1024*1024),
      inputBuffer_(16),
      outputBuffer_(16)
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
        << " fd = " << sockfd;
    sockets::SetKeepAlive(sockfd_, true);
}

TcpConnection::~TcpConnection()
{
    ::sockets::Close(sockfd_);
    LOG << "TcpConnection::dtor[" << name_ << "] at " << this
        << " fd = " << channel_->fd();
}

void* TcpConnection::operator new(size_t size) {
    (void)size;
    return TCPCONN_MEMORY_POOL_.malloc();
}

void TcpConnection::operator delete(void* p) {
    TCPCONN_MEMORY_POOL_.free(p);
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
    if (!channel_->isWriting()) {
        LOG << "Connection fd = " << channel_->fd();
        return;
    }
    ssize_t n = sockets::Write(channel_->fd(),
                               outputBuffer_.data(),
                               outputBuffer_.readableBytes());
    if (n > 0)
    {
        outputBuffer_.retrieve(n);
        if (outputBuffer_.readableBytes() == 0)
        {
            channel_->disableWriting(); //停止关注可写事件,避免出现busy loop
            // if (writeCompleteCallback_) {
            //     loop_->runInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
            // }
            if (state_ == Disconnecting) {//所有的数据都发送完毕，并且有人试图关闭连接，则可以关闭连接
                                          //这个是shutdownInLoop是因为在shutdown()函数中有可能关闭不成功
                shutdownInLoop();         //需要在发送缓冲区发送完毕时再做一次判断
            }
        } else {
            LOG << "data has not been write complete";
        }
    }
    else
    {
        LOG << "TcpConnection::handleWrite()";
    }
}
    

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG << "fd = " << channel_->fd() << " state = " << state_;
    assert(state_ == Connected || state_ == Disconnecting);

    setState(Disconnected);
    channel_->disableAll();

    //用户设置
    connectionCallback_(shared_from_this());
    //TcpServer中设置
    closeCallback_(shared_from_this());
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    LOG << "TcpConnection::handleError [" << name_ 
        << "] -SO_ERROR = " << err << " " << strerror(err);
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
        sockets::ShutdownWrite(sockfd_);
    }
}

//注意，用户调用shutdown函数，只会触发server->client的第一个FIN
//client回复对应的LASTACK后，server会触发handleread, handleclose
//继而会应发TcpServer的removeConnection,
void TcpConnection::shutdown()
{
    if (state_ == Connected)
    {
        //将状态改为Disconnecting
        setState(Disconnecting);
        loop_->runInLoop(boost::bind(&TcpConnection::shutdownInLoop, this));
    }
}

//we don't want to copy data, so it's not thread safe
//do not use this function in other thread
void TcpConnection::send(const std::string& message)
{
    send(message.c_str(), message.size());
}

void TcpConnection::send(const Slice& message)
{
    send(message.data(), message.size());
}

void TcpConnection::send(const Buffer* buf)
{
    // Slice slice(buf->data(), buf->readableBytes());
    // send(slice);
    send(buf->data(), buf->readableBytes());
}

void TcpConnection::send(const void* data, size_t len)
{
    loop_->assertInLoopThread();
    if (state_ == Disconnected)
    {
        LOG << "disconnected, give up left writing";
        return;
    }
    ssize_t nwrote = 0;
    ssize_t nleft = len;
    bool error = false;

    // write directly, if there is nothing in outputBuffer and channel_ has concerned nothing
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = sockets::Write(channel_->fd(), data, len);
        if (nwrote < 0) {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                if (errno == EPIPE) {
                    error = true;
                }
            }
        } else {
            nleft = len - nwrote;
            // if (nleft == 0 && writeCompleteCallback_)
            // {
            //     loop_->queueInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
            //     return;
            // }
        }
    }

    assert(nwrote <= len);
    if (!error && nleft > 0) {
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + nleft >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMarkCallback_)
        {
            loop_->runInLoop(boost::bind(highWaterMarkCallback_, shared_from_this(), oldLen + nleft));
        }
        outputBuffer_.append(static_cast<const char*>(data) + nwrote, nleft);
        channel_->enableWriting();
    }
}


void TcpConnection::setTcpNoDelay(bool on)
{
    ::sockets::SetTcpNoDelay(sockfd_, on);
}

void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == Connecting);
    setState(Connected);

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

