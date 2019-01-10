#ifndef WEBSERVER_UTIL_H
#define WEBSERVER_UTIL_H

#include <stdint.h>
#include <endian.h>
#include <arpa/inet.h>
#include <ctype.h>

namespace ywl 
{
namespace net
{
namespace sockets
{
////////////////////////////////////////////////////////////////////////
//The functions with names of the form "htobenn" convert from
//host byte order to big-endian order.
inline uint64_t hostToNetwork64(uint64_t host64)
{
  return htobe64(host64);
}

inline uint32_t hostToNetwork32(uint32_t host32)
{
  return htobe32(host32);
}

inline uint16_t hostToNetwork16(uint16_t host16)
{
  return htobe16(host16);
}

//The functions with names of the form "benntoh" convert from 
//big-endian order to host byte order.
inline uint64_t networkToHost64(uint64_t net64)
{
  return be64toh(net64);
}

inline uint32_t networkToHost32(uint32_t net32)
{
  return be32toh(net32);
}

inline uint16_t networkToHost16(uint16_t net16)
{
  return be16toh(net16);
}
////////////////////////////////////////////////////////////////////////

void setNonBlockingAndCloseOnExec(int sockfd);
int createNonBlockSocketfd();

int Connect(int sockfd, const struct sockaddr_in& addr);
void Bind(int sockfd, const struct sockaddr_in& addr);
void Listen(int sockfd);
int Accept(int sockfd, struct sockaddr_in* addr);
ssize_t Read(int sockfd, void* buf, size_t count);
ssize_t Readv(int sockfd, const struct iovec* iov, int iovcnt);
ssize_t Write(int sockfd, const void *buf, size_t count);
void Close(int sockfd);
void ShutdownWrite(int sockfd);

ssize_t Readn(int sockfd, void* buf, size_t count);
ssize_t Writen(int sockfd, const void* buf, size_t count);
ssize_t RecvPeek(int sockfd, void* buf, size_t len);
ssize_t ReadLine(int sockfd, void* buf, size_t maxlen);

//把addr中的ip地址从binary中转化到string,并保存才buf中
void toIpString(char* buf, size_t size, const struct sockaddr_in& addr);
void toIpPortString(char* buf, size_t size, const struct sockaddr_in& addr);
void fromIpPortToNetwork(const char* ip, uint16_t port, struct sockaddr_in* addr);

int getSocketError(int sockfd);

struct sockaddr_in getLocalAddr(int sockfd);
struct sockaddr_in getPeerAddr(int sockfd);
bool isSelfConnect(int sockfd);

}

}
}

#endif
