#include <WebServer/Socket.h>
#include <WebServer/Util.h>
#include <WebServer/InetAddress.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>


using namespace ywl;
using namespace ywl::net;

Socket::~Socket()
{
    sockets::Close(sockfd_);
}

void Socket::bindAddress(const InetAddress& addr)
{
    sockets::Bind(sockfd_, addr.getSockAddrInet());
}

void Socket::listen()
{
    sockets::Listen(sockfd_);
}

int Socket::accept(InetAddress* peeraddr)
{
    struct sockaddr_in addr;
    bzero(&addr, sizeof addr);
    int connfd = sockets::Accept(sockfd_, &addr);
    if (connfd >= 0)
    {
        peeraddr->setSockAddrInet(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    sockets::ShutdownWrite(sockfd_);
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}



