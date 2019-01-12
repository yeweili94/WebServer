#ifndef WEBSERVER_SOCKET_H
#define WEBSERVER_SOCKET_H

#include <boost/noncopyable.hpp>

namespace ywl
{
namespace net
{
class InetAddress;

class Socket : boost::noncopyable
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {}
    ~Socket();
    int fd() {return sockfd_; }
    void bindAddress(const InetAddress& localaddr);
    void listen();
     int accept(InetAddress* peeraddr);
    void shutdownWrite();
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};


}
}


#endif
