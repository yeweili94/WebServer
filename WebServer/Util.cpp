#include <WebServer/base/Logging.h>
#include <WebServer/Util.h>

#include <fcntl.h>
#include <sys/socket.h>

typedef struct sockaddr SA;
using namespace ywl;
using namespace ywl::net;

void sockets::setNonBlockingAndCloseOnExec(int sockfd)
{
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = ::fcntl(sockfd, F_SETFL, flags);

    flags = ::fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = ::fcntl(sockfd, F_SETFD, flags);

    (void)ret;
}

int sockets::createNonBlocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        FATAL << "sockets::createNonBlocking()";
    }
    sockets::setNonBlockingAndCloseOnExec(sockfd);
    return sockfd;
}

int sockets::Connect(int sockfd, const struct sockaddr_in& addr)
{
    return ::connect(sockfd, (const SA*)(&addr), sizeof addr);
}

void sockets::Bind(int sockfd, const struct sockaddr_in& addr)
{
    int ret = ::bind(sockfd, (SA*)(&addr), sizeof addr);
    if (ret < 0) {
        FATAL << "socket::Bind()";
    }
}

void sockets::Listen(int sockfd)
{
    int ret = ::listen(sockfd, SOMAXCONN);
    if (ret < 0) {
        FATAL << "sockets::Listen()";
    }
}

int sockets::Accept(int sockfd, struct sockaddr_in* addr)
{
    socklen_t addrlen = sizeof *addr;
    int connfd = accept(sockfd, (SA*)addr, &addrlen);
    sockets::setNonBlockingAndCloseOnExec(connfd);
    if (connfd < 0) {
        LOG << "SYSERR-socket::Accept()";
    }
    return connfd;
}

ssize_t sockets::Read(int sockfd, void *buf, size_t count)
{
    return ::read(sockfd, buf, count);
}

ssize_t sockets::Write(int sockfd, const void* buf, size_t count)
{
    return ::write(sockfd, buf, count);
}

ssize_t sockets::Readv(int sockfd, const struct iovec *iov, int iovcnt)
{
    return ::readv(sockfd, iov, iovcnt);
}

void sockets::Close(int sockfd)
{
    if (::close(sockfd) < 0) {
        LOG << "SYSERR-sockets::Close()";
    }
}

void sockets::ShutdownWrite(int sockfd)
{
    if (::shutdown(sockfd, SHUT_WR) < 0)
    {
        LOG << "SYSERR-sockets::ShutdownWrite()";
    }
}

void sockets::toIpString(char* buf, size_t size, const struct sockaddr_in& addr)
{
    assert(size >= INET_ADDRSTRLEN);
    ::inet_ntop(AF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(size));
}

void sockets::toIpPortString(char* buf, size_t size, const struct sockaddr_in& addr)
{
    char host[INET_ADDRSTRLEN] = "INVALID";
    sockets::toIpString(host, sizeof host, addr);
    //转化为大端主机字节序
    uint16_t port = sockets::networkToHost16(addr.sin_port);
    snprintf(buf, size, "%s:%u", host, port);
}

void sockets::fromIpPortToNetwork(const char* ip, uint16_t port, struct sockaddr_in* addr)
{
    addr->sin_family = AF_INET;
    addr->sin_port = sockets::hostToNetwork16(port);
    if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
    {
        LOG << "SYSERR-sockets::fromIpPortToNetwork()";
    }
}

int sockets::getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = sizeof optval;

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else
    {
        return optval;
    }
}

struct sockaddr_in sockets::getLocalAddr(int sockfd)
{
    struct sockaddr_in localaddr;
    memset(&localaddr, 0, sizeof localaddr);
    socklen_t addrlen = sizeof(localaddr);
    if (::getsockname(sockfd, (SA*)(&localaddr), &addrlen) < 0)
    {
        LOG << "SYSERR-sockets::getLocalAddr()";
    }
    return localaddr;
}

struct sockaddr_in sockets::getPeerAddr(int sockfd)
{
    struct sockaddr_in peeraddr;
    memset(&peeraddr, 0, sizeof peeraddr);
    socklen_t addrlen = sizeof(peeraddr);
    if (::getpeername(sockfd, (SA*)(&peeraddr), &addrlen) < 0)
    {
        LOG << "SYSERR-sockets::getPeerAddr()";
    }
    return peeraddr;
}

bool sockets::isSelfConnect(int sockfd)
{
    struct sockaddr_in localaddr = getLocalAddr(sockfd);
    struct sockaddr_in peeraddr = getPeerAddr(sockfd);
    return localaddr.sin_port == peeraddr.sin_port
        && localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
}

