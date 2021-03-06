#include <WebServer/TcpServer.h>
#include <WebServer/EventLoop.h>
#include <WebServer/InetAddress.h>
#include <WebServer/base/Timestamp.h>
#include <signal.h>

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
    // printf("onMessage(): reveived %zd bytes from connection [%s]\n",
           // buf->readableBytes(), conn->name().c_str());
    (void)receiveTime;
    // printf("buffer current data size is : %zu\n", buf->length());
    // printf("buffer readerableBytes is : %zu\n", buf->readableBytes());
    // size_t len = buf->length();
    // std::string str = buf->nextAllString();
    // printf("buffer writeableBytes is : %zu\n", buf->writeableBytes());
    // printf("send message:%s\n", str.c_str());
    // conn->send(str);
    buf->reset();
    (void)conn;
    // conn->shutdown();
}

int main()
{
    ::signal(SIGPIPE, SIG_IGN);
    ::signal(SIGABRT, SIG_IGN);
    printf("main(): pid = %d\n", ::getpid());

    printf("sizeof TcpConnection is %lu\n", sizeof(TcpConnection));
    printf("sizeo Channel is %lu\n", sizeof(Channel));
    InetAddress listenAddr(8900);
    EventLoop loop;

    TcpServer server(&loop, listenAddr, "TestServer");
    server.setThreadNum(4);
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    server.start();

    loop.loop();
}
