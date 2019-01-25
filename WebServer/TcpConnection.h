#ifndef WEBSERVER_TCPCONNECTION_H
#define WEBSERVER_TCPCONNECTION_H

#include <WebServer/Channel.h>
#include <WebServer/InetAddress.h>
#include <WebServer/EventLoop.h>
#include <WebServer/Buffer.h>
#include <WebServer/base/Timestamp.h>
#include <WebServer/Slice.h>
#include <WebServer/MemoryPool.h>

#include <boost/enable_shared_from_this.hpp>

namespace ywl
{
namespace net
{
//////////////////////////////////////////////////////////////////////////////
class TcpConnection;
typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef boost::function<void(const TcpConnectionPtr&)> ConnectionCallback;
typedef boost::function<void(const TcpConnectionPtr&)> CloseCallback;
typedef boost::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)> MessageCallback;

/////////////////////////////////////////////////////////////////////////////
class TcpConnection : public boost::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop,
                     const std::string& name,
                     int sockfd,  
                     const InetAddress& localAddr,
                     const InetAddress& peerAddr);
    ~TcpConnection();

    void* operator new(size_t size);
    void operator delete(void* p);

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() { return localAddr_; }
    const InetAddress& peerAddress() { return peerAddr_; }
    bool connected() const { return state_ == Connected; }
    //send data to peer client
    void send(const void* message, size_t len);
    void send(const std::string& message);
    void send(const Slice& message);
    void send(const Buffer* buf);
    void shutdown();
    void setTcpNoDelay(bool on);

    void setContex(const boost::any& contex){ contex_ = contex; }
    const boost::any& getContex() const { return contex_; }
    boost::any* getMutableContex() { return &contex_; }

    void setConnectionCallback(const ConnectionCallback& cb) { 
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback& cb) { 
        messageCallback_ = cb;
    }
    void setCloseCallback(const CloseCallback& cb) {
        closeCallback_ = cb;
    }
    void connectEstablished();
    void connectDestroyed();

    Buffer* inputBuffer() { return &inputBuffer_; }

private:
    enum StateE {Connecting, Connected, Disconnecting, Disconnected};

    void setState(StateE s) { state_ = s; }
    //设置channel的各种事件回调函数
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void shutdownInLoop();

    EventLoop* loop_;
    StateE state_;
    std::string name_;

    int sockfd_;
    boost::scoped_ptr<Channel> channel_;

    InetAddress localAddr_;
    InetAddress peerAddr_;

    ConnectionCallback connectionCallback_; //TcpServer中设置(必须)
    MessageCallback messageCallback_;   //TcpServer中设置(必须)
    CloseCallback closeCallback_;   //TcpServer中设置(必须)

    Buffer inputBuffer_;
    Buffer outputBuffer_;

    boost::any contex_;
    
};



}
}

#endif
