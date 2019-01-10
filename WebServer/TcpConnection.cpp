#include <WebServer/TcpConnection.h>
#include <WebServer/base/Logging.h>
#include <WebServer/Channel.h>
#include <WebServer/Util.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <stdio.h>

using namespace ywl;
using namespace ywl::net;

void TcpConnection::handleRead()
{
}
