#include <WebServer/EventLoop.h>
#include <WebServer/EventLoopThread.h>
#include <WebServer/TcpClient.h>
#include <WebServer/InetAddress.h>

using namespace ywl;
using namespace ywl::net;
int main()
{
    EventLoop loop;
    // EventLoopThreadPool pool(&loop);
    // pool.setThreadNum(1);
    InetAddress servAddr("127.0.0.1", 8900);
    TcpClient client(&loop, servAddr, "client");
    client.start();
    client.send("hellow");
    sleep(1);
    loop.loop();
}
