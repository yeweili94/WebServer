#include <WebServer/EventLoop.h>
#include <WebServer/Acceptor.h>
#include <WebServer/InetAddress.h>
#include <WebServer/Util.h>

#include <stdio.h>

using namespace ywl;
using namespace ywl::net;

void newConnection(int sockfd, const InetAddress& peerAddr)
{
    printf("newConnection(): accepted a new connection from %s\n",
           peerAddr.toIpPort().c_str());
    int n = ::write(sockfd, "How are you?\n", 13);
    (void)n;
    // sockets::Close(sockfd);
}

int main()
{
    printf("main(): pid = %d\n", getpid());

    InetAddress listenAddr(8099);
    EventLoop loop;

    Acceptor acceptor(&loop, listenAddr);
    acceptor.setNewConnectionCallback(newConnection);
    acceptor.listen();

    loop.loop();
}
