#ifndef WEBSERVER_INETADDRESS_H
#define WEBSERVER_INETADDRESS_H

#include <WebServer/Util.h>

#include <netinet/in.h>
#include <strings.h>

#include <string>

namespace ywl
{
namespace net
{

//wrapper of sockaddr_in.
class InetAddress
{
public:
    explicit InetAddress(uint16_t port);
    InetAddress(const struct sockaddr_in& addr)
        : addr_(addr)
    {}
    std::string toIp() const;
    std::string toIpPort() const;
    const struct sockaddr_in& getSockAddrInet() const { return addr_; }
    void setSockAddrInet(const struct sockaddr_in& addr) { addr_ = addr; }
    uint32_t ipNetEndian() const { return addr_.sin_addr.s_addr; }
    uint16_t portNetEndian() const { return addr_.sin_port; }
private:
    struct sockaddr_in addr_;
};

InetAddress::InetAddress(uint16_t port)
{
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = sockets::hostToNetwork32(INADDR_ANY);
    addr_.sin_port = sockets::hostToNetwork16(port);
}

std::string InetAddress::toIpPort () const
{
    char buf[32];
    sockets::toIpPortString(buf, sizeof buf, addr_);
    return buf;
}

std::string InetAddress::toIp() const
{
    char buf[32];
    sockets::toIpString(buf, sizeof buf, addr_);
    return buf;
}

}
}


#endif
