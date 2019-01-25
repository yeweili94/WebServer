#include <WebServer/TcpClient.h>
#include <WebServer/base/Logging.h>
#include <WebServer/EventLoop.h>
#include <WebServer/Util.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace ywl;
using namespace ywl::net;

namespace detail
{
void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buf, Timestamp timestamp)
{
    (void) timestamp;
    // Slice slice = buf->nextAll();
    // std::string str = buf->nextAllString();
    // conn->send(str);
}

void defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    (void) conn;
    printf("connection\n");
}

}

TcpClient::TcpClient(EventLoop* loop,
                     const InetAddress& serverAddr,
                     const std::string& name)
    : loop_(loop),
      serverAddr_(serverAddr),
      sockFd_(::sockets::createNonBlockSocketfd()),
      name_(name),
      started_(false),
      connectionCallback_(::detail::defaultConnectionCallback),
      messageCallback_(::detail::defaultMessageCallback)
{
    LOG << "TcpClient::TcpClient[" << name_
        << "] - ctor " << this;
}

TcpClient::~TcpClient()
{
    sockets::Close(sockFd_);
    LOG << "TcpClient::TcpClient[" << name_
        << "] - dctor " << this;
}

void TcpClient::start()
{
    started_ = true;
    connect();
}

void TcpClient::connect()
{
    sockets::Connect(sockFd_, serverAddr_.getSockAddrInet());
    InetAddress peerAddr(sockets::getPeerAddr(sockFd_));
    InetAddress localAddr(sockets::getLocalAddr(sockFd_));

    conn_.reset(new TcpConnection(loop_,
                                  name_,
                                  sockFd_,
                                  localAddr,
                                  peerAddr)); 
    assert(conn_.use_count() == 1);

    conn_->setConnectionCallback(connectionCallback_);
    conn_->setMessageCallback(messageCallback_);
    conn_->connectEstablished();
}

void TcpClient::send(const std::string& message)
{
    conn_->send(message);
}

void TcpClient::shutdown()
{
    conn_->shutdown();
}
