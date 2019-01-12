#ifndef WEBSERVER_INETADDRESS_H
#define WEBSERVER_INETADDRESS_H

#include <netinet/in.h>

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
    InetAddress(const std::string& ip, uint16_t port);
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


}
}


#endif
