#ifndef WEBSERVER_HTTPSERVER_H
#define WEBSERVER_HTTPSERVER_H

#include <WebServer/TcpServer.h>
#include <boost/noncopyable.hpp>


namespace ywl
{
namespace net
{

class HttpRequest;
class HttpResponse;

class HttpServer : boost::noncopyable
{
public:
    HttpServer(EventLoop* loop,
               const InetAddress& listenAddr,
               const std::string& name);
    ~HttpServer();

    void setThreadNumber(int numThreads) {
        tcpServer_.setThreadNum(numThreads);
    }

    void start();

private:
    void httpCallback(const HttpRequest&, HttpResponse*);
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn,
                   Buffer* buf,
                   Timestamp receiveTime);

    void onRequest(const TcpConnectionPtr&, const HttpRequest&);

    TcpServer tcpServer_;
};

}
}

#endif
