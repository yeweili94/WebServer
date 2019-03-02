#include <WebServer/http/HttpServer.h>
#include <WebServer/http/HttpResponse.h>
#include <WebServer/http/HttpRequest.h>
#include <WebServer/http/HttpParser.h>
#include <WebServer/base/Logging.h>

#include <boost/bind.hpp>

#include <sys/stat.h>
#include <iostream>

using namespace ywl;
using namespace ywl::net;

namespace ywl
{
namespace net
{

static void read_file(FILE* fp, char** output, int* length)
{
    struct stat filestats;
    int fd = fileno(fp);
    fstat(fd, &filestats);
    *length = filestats.st_size;
    *output = (char*)malloc(*length+1);
    int start = 0;
    int bytes_read;
    while ((bytes_read = fread(*output + start, 1, *length - start, fp))) {
        start += bytes_read;
    }
}

HttpServer::HttpServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       const std::string& name,
                       int idleSeconds)
    : tcpServer_(loop, listenAddr, name),
      connectionBuckets_(idleSeconds)
    {
        tcpServer_.setConnectionCallback(
            boost::bind(&HttpServer::onConnection, this, _1));
        tcpServer_.setMessageCallback(
            boost::bind(&HttpServer::onMessage, this, _1, _2, _3));
        connectionBuckets_.resize(idleSeconds);
        loop->runEvery(1.0, boost::bind(&HttpServer::onTimer, this));
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

void HttpServer::onTimer()
{
    connectionBuckets_.push_back(Bucket());
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        conn->setContex(HttpParser());
        EntryPtr entry(new Entry(conn));
        connectionBuckets_.back().insert(entry);
        boost::weak_ptr<Entry> weakEntry(entry);
        conn->setTimerNode(weakEntry);
    }
}

void HttpServer::onMessage(const TcpConnectionPtr& conn,
                              Buffer* buf,
                              Timestamp receiveTime)
{
    //handle timer wheel
    if (conn->connected()) {
        EntryPtr entry = boost::any_cast<boost::weak_ptr<Entry>>(conn->getTimerNode()).lock();
        if (entry) {
            connectionBuckets_.back().insert(entry);
        }
    }
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
        if (connection != "keep-alive" && connection != "Keep-Alive")
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
    // std::cout << "Headers " << req.methodString() << " " << req.path() << std::endl;
    // auto& headers = req.headers();
    // for (auto it = headers.begin(); it != headers.end(); it++) {
        // std::cout << it->first << ": " << it->second << std::endl;
    // }
    if (req.path() == "/") {
        resp->setStatusCode(HttpResponse::k200OK);
        resp->setContentType("text/html");
        resp->setStatusMessage("OK");
        resp->addHeader("server", "Chord");
        std::string now = Timestamp::now().toFormattedString();
        resp->setBody("<html><head><title>This is title</title></head>"
                "<body><h1>Hello</h1>Now is " + now + "</body></html>");
    }
    else if (req.path() == "/baidu") {
        loadPage("page/baidu.html", resp);
    }
    else if (req.path() == "/taobao") {
        loadPage("page/taobao.html", resp);
    }
    else if (req.path() == "/interview") {
        loadPage("page/interview.html", resp);
    }
    else if (req.path() == "/bupt") {
        loadPage("page/bupt.html", resp);
    }
    else if (req.path() == "/google") {
        loadPage("page/google.html", resp);
    }
    else if (req.path() == "/byrbbs") {
        loadPage("page/byrbbs.html", resp);
    }
    else if (req.path() == "/cpp.png") {
        resp->setStatusCode(HttpResponse::k200OK);
        resp->setStatusMessage("OK");
        resp->setContentType("png");
        resp->addHeader("Server", "Chord");

        FILE* fp = fopen("page/542378.png", "r");
        char* input;
        int input_len;
        if (fp) {
            read_file(fp, &input, &input_len);
        } else 
        {
            ::fclose(fp);
            return;
        }
        resp->setBody(std::string(input, input_len));
        ::fclose(fp);
        free(input);
    }
    else if (req.path() == "/huge.jpg") {
        resp->setStatusCode(HttpResponse::k200OK);
        resp->setStatusMessage("OK");
        resp->setContentType("jpg");
        resp->addHeader("Server", "Chord");

        FILE* fp = fopen("page/huge.jpg", "r");
        char* input;
        int input_len;
        if (!fp) return;
        read_file(fp, &input, &input_len);
        resp->setBody(std::string(input, input_len));
        ::fclose(fp);
        free(input);
    }
    else
    {
        resp->setStatusCode(HttpResponse::k404NotFound);
        resp->setStatusMessage("Not Found");
        resp->setCloseConnection(true);
    }
}

void HttpServer::loadPage(const std::string& path, HttpResponse* resp)
{
    FILE* fp = ::fopen(path.c_str(), "r");
    if (!fp) {
        resp->setStatusCode(HttpResponse::k404NotFound);
        resp->setStatusMessage("Not Found");
        resp->setCloseConnection(true);
    }
    else
    {
        resp->setStatusCode(HttpResponse::k200OK);
        resp->setStatusMessage("OK");
        resp->setContentType("text/html");
        resp->addHeader("Server", "Chord");

        char* input;
        int input_len;
        read_file(fp, &input, &input_len);
        resp->setBody(std::string(input, input_len));
        ::fclose(fp);
        free(input);
    }
}

}
}


using namespace ywl;
using namespace ywl::net;

int main()
{
    EventLoop loop;
    HttpServer server(&loop, InetAddress(8900), "dummy", 8);
    server.setThreadNumber(4);
    server.start();
    loop.loop();
}



