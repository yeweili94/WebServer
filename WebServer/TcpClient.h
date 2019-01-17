#ifndef WEBSERVER_TCPCLIENT_H
#define WEBSERVER_TCPCLIENT_H

#include <WebServer/TcpConnection.h>
#include <WebServer/Util.h>
#include <WebServer/InetAddress.h>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

namespace ywl
{
namespace net
{

class TcpClient : boost::noncopyable
{
public:
    typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

    TcpClient(EventLoop* loop,
              const InetAddress& serverAddr,
              const std::string& name_);

    ~TcpClient();

    void start();
    void shutdown();
    void send(const std::string& message);

    TcpConnectionPtr connection() const {
        return conn_;
    }

    void setConnectionCallback(const ConnectionCallback& cb) {
        connectionCallback_ = cb;
    }

    void setMessageCallback(const MessageCallback& cb) {
        messageCallback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {
        writeCompleteCallback_ = cb;
    }

private:
    void connect();
    EventLoop* loop_;
    InetAddress serverAddr_;
    int sockFd_;
    TcpConnectionPtr conn_;
    std::string name_;
    bool started_;
    
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
};

}
}

#endif
