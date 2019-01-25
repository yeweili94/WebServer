#ifndef WEBSERVER_TCPSERVER_H
#define WEBSERVER_TCPSERVER_H

#include <WebServer/InetAddress.h>
#include <WebServer/TcpConnection.h>
#include <WebServer/EventLoopThread.h>

#include <map>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace ywl
{
namespace net
{

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

class TcpServer : boost::noncopyable
{
public:
    TcpServer(EventLoop* mainLoop,
              const InetAddress& listenAddr,
              const std::string& name);
    ~TcpServer();

    const std::string& hostPort() const { return hostPort_; }
    const std::string& name() const { return name_; }
    void setThreadNum(int threadNum);

    void setThreadInitCallback(const ThreadInitCallback& cb)
    {
        threadInitCallback_ = cb;
    }

    void start();

    void setConnectionCallback(const ConnectionCallback& cb)
    {
        connectionCallback_ = cb;
    }
    //消息到来函数
    //typedef boost::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)> MessageCallback;
    void setMessageCallback(const MessageCallback& cb)
    {
        messageCallback_ = cb;
    }
private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInloop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::map<std::string, TcpConnectionPtr>;

    EventLoop* mainLoop_;
    boost::scoped_ptr<Acceptor> acceptor_;
    boost::scoped_ptr<EventLoopThreadPool> threadPool_;

    std::string hostPort_;
    std::string name_;
    ThreadInitCallback threadInitCallback_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;

    bool started_;
    int nextConnId_;
    ConnectionMap connections_;
};
}
}

#endif
