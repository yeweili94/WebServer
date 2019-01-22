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
//��TcpServer�е�Acceptor�����õ�NewConnectionCallback_��sockfd
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
    //channel�����ɶ��¼����ص�handleRead()����
    channel_->setReadCallback(
        boost::bind(&TcpConnection::handleRead, this, _1));
    //channel������д�¼����ص�handleWrite()����
    channel_->setWriteCallback(
        boost::bind(&TcpConnection::handleWrite, this));
    //���ӹرգ��ص�haneleClose()����
    channel_->setCloseCallback(
        boost::bind(&TcpConnection::handleClose, this));
    //�������󣬻ص�handleError()����
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
            channel_->disableWriting(); //ֹͣ��ע��д�¼�,�������busy loop
            // if (writeCompleteCallback_) {
            //     loop_->runInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
            // }
            if (state_ == Disconnecting) {//���е����ݶ�������ϣ�����������ͼ�ر����ӣ�����Թر�����
                                          //�����shutdownInLoop����Ϊ��shutdown()�������п��ܹرղ��ɹ�
                shutdownInLoop();         //��Ҫ�ڷ��ͻ������������ʱ����һ���ж�
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

    //�û�����
    connectionCallback_(shared_from_this());
    //TcpServer������
    closeCallback_(shared_from_this());
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    LOG << "TcpConnection::handleError [" << name_ 
        << "] -SO_ERROR = " << err << " " << strerror(err);
}


//��Ӧ�ò���Ҫ�ر����ӣ�����ֱ�ӵ���handleclose
//��Ϊ�п���outputBuffer�е����ݻ�û�з�����
//Ҳ����˵conn->send()ֻҪ����û�й��Ͼ�Ӧ�ñ�֤�����ķ���������Ϣ
void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    //������ڹ�עPOLLOUT�¼���Ҳ����˵��������û�з�����,��ʱ�����ǲ��ܹر�����
    //��ʱ��shutdownWriteӦ����handleWrite�����ݷ�����Ϻ��ٵ���һ���������
    //��ʱ�жϵ�������tcpconnection��״̬,�Ƿ�ΪDisconnecting
    if (!channel_->isWriting())
    {
        sockets::ShutdownWrite(sockfd_);
    }
}

//ע�⣬�û�����shutdown������ֻ�ᴥ��server->client�ĵ�һ��FIN
//client�ظ���Ӧ��LASTACK��server�ᴥ��handleread, handleclose
//�̶���Ӧ��TcpServer��removeConnection,
void TcpConnection::shutdown()
{
    if (state_ == Connected)
    {
        //��״̬��ΪDisconnecting
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

