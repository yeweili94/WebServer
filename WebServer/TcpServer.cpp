#include <WebServer/TcpServer.h>
#include <WebServer/TcpConnection.h>
#include <WebServer/Acceptor.h>
#include <WebServer/Socket.h>
#include <WebServer/EventLoop.h>
#include <WebServer/base/Logging.h>

#include <boost/bind.hpp>
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

TcpServer::TcpServer(EventLoop* acceptLoop,
                     const InetAddress& listenAddr,
                     const std::string& name)
    : acceptLoop_(acceptLoop),
      acceptor_(new Acceptor(acceptLoop_, listenAddr)),
      threadPool_(new EventLoopThreadPool(acceptLoop_)),
      hostPort_(listenAddr.toIpPort()),
      name_(name),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      started_(false),
      nextConnId_(1)
{
    //����Ӧ����socket������������Ӧ���ǶԵȷ��ĵ�ַ
    acceptor_->setNewConnectionCallback(
            boost::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
    acceptLoop_->assertInLoopThread();
    LOG << "TcpServer::~TcpServer [" << name_ << "] destructing";

    for (ConnectionMap::iterator it(connections_.begin()); it != connections_.end(); ++it)
    {
        TcpConnectionPtr conn = it->second;
        it->second.reset(); //�ͷ�TcpConnection����
        conn->getLoop()->runInLoop(
            boost::bind(&TcpConnection::connectDestroyed, conn));
        conn.reset();
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    assert(0 <= numThreads);
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if (!started_)
    {
        started_ = true;
        threadPool_->start(threadInitCallback_);
    }

    if (!acceptor_->listenning())
    {
        // acceptLoop_->runInLoop(
            // std::bind(&Acceptor::listen, get_pointer(acceptor_)));
        acceptor_->listen();
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    acceptLoop_->assertInLoopThread();
    EventLoop* ioLoop = threadPool_->getNextLoop();
    LOG << "nextLoop is" << nextConnId_;
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", hostPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG << "TcpServer::newConnection [" << name_
        << "] - new Connection [" << connName
        << "] from" << peerAddr.toIpPort();
    InetAddress localAddr(sockets::getLocalAddr(sockfd));

    TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));

    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    LOG << "Set connectioncallback";
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    conn->setCloseCallback(
        boost::bind(&TcpServer::removeConnection, this, _1));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    acceptLoop_->runInLoop(boost::bind(&TcpServer::removeConnectionInloop, this, conn));
}

void TcpServer::removeConnectionInloop(const TcpConnectionPtr& conn)
{
    acceptLoop_->assertInLoopThread();
    LOG << "TcpServer::removeConnectionInLoop [" << name_
        << "] - connection " << conn->name();
    size_t n = connections_.erase(conn->name());
    assert(n == 1);
    (void)n;

    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        boost::bind(&TcpConnection::connectDestroyed, conn));
}


