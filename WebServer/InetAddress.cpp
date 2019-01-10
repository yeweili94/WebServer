#include <WebServer/InetAddress.h>
#include <WebServer/Util.h>

#include <strings.h>

using namespace ywl;
using namespace ywl::net;

InetAddress::InetAddress(uint16_t port)
{
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = sockets::hostToNetwork32(INADDR_ANY);
    addr_.sin_port = sockets::hostToNetwork16(port);
}

InetAddress::InetAddress(const std::string& ip, uint16_t port)
{
    bzero(&addr_, sizeof addr_);
    sockets::fromIpPortToNetwork(ip.c_str(), port, &addr_);
}

std::string InetAddress::toIpPort() const
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
