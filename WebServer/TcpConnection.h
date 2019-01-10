#ifndef WEBSERVER_TCPCONNECTION_H
#define WEBSERVER_TCPCONNECTION_H

#include <WebServer/Channel.h>
#include <WebServer/Socket.h>
#include <WebServer/InetAddress.h>
#include <WebServer/EventLoop.h>
#include <WebServer/Buffer.h>
#include <WebServer/base/Timestamp.h>

#include <boost/enable_shared_from_this.hpp>

namespace ywl
{
namespace net
{

class TcpConnection;
typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

typedef boost::function<void(const TcpConnectionPtr&)> ConnectionCallback;
typedef boost::function<void(const TcpConnectionPtr&)> CloseCallback;
typedef boost::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
typedef boost::function<void(const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;
typedef boost::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)> MessageCallback;

/////////////////////////////////////////////////////////////////////////////
class TcpConnection : boost::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop,
                     const std::string& name,
                     int sockfd,
                     const InetAddress& localAddr,
                     const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() { return localAddr_; }
    const InetAddress& peerAddress() { return peerAddr_; }
    bool connected() const { return state_ == Connected; }
    void send(const void* message, size_t len);
    void send(Buffer* message);
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
    void setWriteCompleteCallback(const ConnectionCallback& cb){
        writeCompleteCallback_ = cb;
    }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
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
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();
    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    EventLoop* loop_;
    StateE state_;
    std::string name_;
    boost::scoped_ptr<Socket> socket_;
    boost::scoped_ptr<Channel> channel_;
    InetAddress localAddr_;
    InetAddress peerAddr_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    //���е��û����ݶ��������ں˻�����ʱ���øûص�����
    //outputBuffer�����Ҳ��ص��ú���
    WriteCompleteCallback writeCompleteCallback_;
    //��ˮλ��־�ص�����
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;  //��ˮλ��
    Buffer inputBuffer_;
    Buffer outputBuffer_;
    boost::any contex_;
};

}
}

#endif
