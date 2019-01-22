#ifndef WEBSERVER_HTTPPARSER_H
#define WEBSERVER_HTTPPARSER_H

#include <WebServer/http/HttpRequest.h>

namespace ywl
{
namespace net
{

class Buffer;
class HttpParser
{
public:
    enum HttpRequestParseState
    {
        kExpectRequestLine,
        kExpectHeaders,
        kGotAll
    };

    HttpParser()
        : state_(kExpectRequestLine),
          request_()
    {

    }

    bool expectRequestLine() const
    {
        return state_ == kExpectRequestLine; 
    }

    void receiveRequestLine()
    {
        state_ = kExpectHeaders;
    }

    bool expectHeaders() const
    {
        return state_ == kExpectHeaders;
    }

    void receiveHeaders() 
    {
        state_ = kGotAll;
    }

    bool gotAll() const
    {
        return state_  == kGotAll;
    }

    void reset()
    {
        state_ = kExpectRequestLine;
        HttpRequest dummy;
        request_.swap(dummy);
    }

    const HttpRequest& request() const
    {
        return request_;
    }

    HttpRequest& request()
    {
        return request_;
    }

    bool parseRequest(Buffer* buf, Timestamp receiveTime);

private:
    bool processRequestLine(const char* begin, const char* end);

    HttpRequestParseState state_; //http«Î«ÛΩ‚Œˆ◊¥Ã¨
    HttpRequest request_;   //http«Î«Û
};

}
}

#endif
