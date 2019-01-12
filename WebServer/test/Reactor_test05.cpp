#include <WebServer/TcpServer.h>
#include <WebServer/EventLoop.h>
#include <WebServer/InetAddress.h>
#include <WebServer/base/Timestamp.h>

#include <stdio.h>

using namespace ywl;
using namespace ywl::net;

void onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        printf("onConnection() : new Connection [%s] form %s\n",
                conn->name().c_str(),
                conn->peerAddress().toIpPort().c_str());
    }
    else
    {
        printf("onConnection(): connection [%s] is down\n",
               conn->name().c_str());
    }
}

void onMessage(const TcpConnectionPtr& conn,
               Buffer* buf,
               Timestamp receiveTime)
{
    printf("onMessage(): reveived %zd bytes from connection [%s]\n",
           buf->readableBytes(), conn->name().c_str());
    (void)buf;
    (void)receiveTime;
}

int main()
{
    printf("main(): pid = %d\n", ::getpid());

    InetAddress listenAddr(8900);
    EventLoop loop;

    TcpServer server(&loop, listenAddr, "TestServer");
    server.setThreadNum(4);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.start();

    loop.loop();
}
