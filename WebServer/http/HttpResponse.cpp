#include <WebServer/http/HttpResponse.h>
#include <WebServer/Buffer.h>

using namespace ywl;
using namespace ywl::net;

void HttpResponse::appendToBuffer(Buffer* outputbuf) const
{
    char buf[32];
    snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statusCode_);
    outputbuf->append(buf);
    outputbuf->append(statusMessage_);
    outputbuf->append("\r\n");

    if (closeConnection_)
    {
        outputbuf->append("Connection: close\r\n");
    }
    else
    {
        snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size()); //实体长度
        outputbuf->append(buf);
        outputbuf->append("Connection: Keep-Alive\r\n");
    }

    for (std::map<std::string, std::string>::const_iterator it = headers_.begin();
         it != headers_.end(); ++it)
    {
        outputbuf->append(it->first);
        outputbuf->append(": ");
        outputbuf->append(it->second);
        outputbuf->append("\r\n");
    }

    outputbuf->append("\r\n");  //body与header之间有一个\r\n
    outputbuf->append(body_);
}
