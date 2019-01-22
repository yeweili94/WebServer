#include <WebServer/http/HttpServer.h>
#include <WebServer/http/HttpResponse.h>
#include <WebServer/http/HttpRequest.h>
#include <WebServer/http/HttpParser.h>
#include <WebServer/base/Logging.h>

#include <boost/bind.hpp>

#include <iostream>

using namespace ywl;
using namespace ywl::net;

namespace ywl
{
namespace net
{

HttpServer::HttpServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       const std::string& name)
    : tcpServer_(loop, listenAddr, name)
    {
        tcpServer_.setConnectionCallback(
            boost::bind(&HttpServer::onConnection, this, _1));
        tcpServer_.setMessageCallback(
            boost::bind(&HttpServer::onMessage, this, _1, _2, _3));
    }

HttpServer::~HttpServer()
{

}

void HttpServer::start()
{
    LOG << "HttpServer[" << tcpServer_.name()
        << "] starts listening on " << tcpServer_.hostPort();
    tcpServer_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        conn->setContex(HttpParser());
    }
}

void HttpServer::onMessage(const TcpConnectionPtr& conn,
                              Buffer* buf,
                              Timestamp receiveTime)
{
    std::cout << std::string(buf->data(), buf->readableBytes()) << std::endl;
    HttpParser* parser = boost::any_cast<HttpParser>(conn->getMutableContex());
    if (!parser->parseRequest(buf, receiveTime))
    {
        std::string message("Http/1.1 400 Bad Request\r\n\r\n");
        conn->send(message);
        conn->shutdown();
    }

    if (parser->gotAll())
    {
        //onRequest
        onRequest(conn, parser->request());
        parser->reset();
    }
}

void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req)
{
    const std::string& connection = req.getHeader("Connection");
    bool close = false;
    if (connection == "close")
    {
        close = true;
    }
    else if (req.getVersion() == HttpRequest::kHttp10) 
    {
        if (connection == "keep-alive" || connection == "Keep-Alive")
        {
            close = false;
        }
        else
        {
            close = true;
        }
    }
    //创建一个HttpResponse
    HttpResponse response(close);
    //httpcallback
    httpCallback(req, &response);
    Buffer buf;
    response.appendToBuffer(&buf);
    conn->send(&buf);
    if (response.iscloseConnection())
    {
        conn->shutdown();
    }
}

void HttpServer::httpCallback(const HttpRequest& req, HttpResponse* resp)
{
    std::cout << "Headers " << req.methodString() << " " << req.path() << std::endl;
    auto& headers = req.headers();
    for (auto it = headers.begin(); it != headers.end(); it++) {
        std::cout << it->first << ": " << it->second << std::endl;
    }
    if (req.path() == "/") {
        resp->setStatusCode(HttpResponse::k200OK);
        resp->setStatusMessage("OK");
        resp->setContentType("text/html");
        resp->addHeader("Server", "WEBSERVER");
        std::string now = Timestamp::now().toFormattedString();
        // std::string body = "\
        //                     <html>\
        //                         <head>\
        //                             <title>This is title</title>\
        //                         </head>\
        //                         <body>\
        //                             <h1>Hello, world</h1>\
        //                             Now is " + now +"\
        //                         </body>\
        //                     </html>";
        std::string body = "<html><head><title>This is title</title></head><body><h1>Hello</h1>Now is" + now + "</body></html>";
        resp->setBody(body);
    }
    else
    {
        resp->setStatusCode(HttpResponse::k404NotFound);
        resp->setStatusMessage("Not Found");
        resp->setCloseConnection(true);
    }
}

}
}


using namespace ywl;
using namespace ywl::net;

int main()
{
    EventLoop loop;
    HttpServer server(&loop, InetAddress(8900), "dummy");
    server.setThreadNumber(8);
    server.start();
    loop.loop();
}



