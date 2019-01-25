#include <WebServer/TcpServer.h>
#include <WebServer/TcpConnection.h>
#include <WebServer/Acceptor.h>
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
    fprintf(stderr, "%s\n", std::string(buf->data(), buf->readableBytes()).c_str());
}

}
}

using namespace ywl;
using namespace ywl::net;

TcpServer::TcpServer(EventLoop* mainLoop,
                     const InetAddress& listenAddr,
                     const std::string& name)
    : mainLoop_(mainLoop),
      acceptor_(new Acceptor(mainLoop_, listenAddr)),
      threadPool_(new EventLoopThreadPool(mainLoop_)),
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
    mainLoop_->assertInLoopThread();
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
        acceptor_->listen();
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    mainLoop_->assertInLoopThread();
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
    // conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        boost::bind(&TcpServer::removeConnection, this, _1));
    conn->getLoop()->runInLoop(boost::bind(&TcpConnection::connectEstablished, conn));
}

//���TcpConnection�����������ӳ�TcpConnection���������ڵ�
//�����������IO�߳��е��õģ�����Ӧ����runInLoop���뵽mainLoop�߳���ȥ����
void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    mainLoop_->runInLoop(boost::bind(&TcpServer::removeConnectionInloop, this, conn));
}

void TcpServer::removeConnectionInloop(const TcpConnectionPtr& conn)
{
    mainLoop_->assertInLoopThread();
    LOG << "TcpServer::removeConnectionInLoop [" << name_
        << "] - connection " << conn->name();
    assert(conn.use_count() == 2);
    size_t n = connections_.erase(conn->name());
    assert(conn.use_count() == 1);
    assert(n == 1); (void)n;
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        boost::bind(&TcpConnection::connectDestroyed, conn));
}



