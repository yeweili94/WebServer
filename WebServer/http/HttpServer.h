#ifndef WEBSERVER_HTTPSERVER_H
#define WEBSERVER_HTTPSERVER_H

#include <WebServer/TcpServer.h>

#include <boost/noncopyable.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>


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
               const std::string& name,
               int idleSeconds);
    ~HttpServer();

    void setThreadNumber(int numThreads) {
        tcpServer_.setThreadNum(numThreads);
    }

    void start();

private:
    void loadPage(const std::string& path, HttpResponse* resp);
    void httpCallback(const HttpRequest&, HttpResponse*);
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn,
                   Buffer* buf,
                   Timestamp receiveTime);
    void onRequest(const TcpConnectionPtr&, const HttpRequest&);
    void onTimer();
    void dumpConnectionBuckets() const;

    typedef boost::weak_ptr<TcpConnection> WeakTcpConnectionPtr;

    TcpServer tcpServer_;

private:
    struct Entry
    {
        explicit Entry(const TcpConnectionPtr& weakConn)
            : weakConn_(weakConn) { }
        ~Entry()
        {
           TcpConnectionPtr conn(weakConn_.lock());
           if (conn) {
               conn->shutdown();
           }
        }

        WeakTcpConnectionPtr weakConn_;
    };

    typedef boost::shared_ptr<Entry> EntryPtr;
    typedef boost::unordered_set<EntryPtr> Bucket;
    typedef boost::circular_buffer<Bucket> WeakConnectionList;

    WeakConnectionList connectionBuckets_;
};

}
}

#endif
