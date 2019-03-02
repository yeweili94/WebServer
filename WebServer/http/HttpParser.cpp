#include <WebServer/http/HttpParser.h>
#include <WebServer/Buffer.h>

using namespace ywl;
using namespace ywl::net;

bool HttpParser::parseRequest(Buffer* buf, Timestamp receiveTime)
{
    bool ok = true;
    bool notFinished = true;
    while (notFinished)
    {
        if (expectRequestLine())
        {
            const char* crlf = buf->findCRLF();
            if (crlf)
            {
                ok = processRequestLine(buf->data(), crlf); //请求行
                if (ok)
                {
                    request_.setReceiveTime(receiveTime);   //请求时间
                    size_t len = crlf - buf->data() + 2;
                    buf->retrieve(len);
                    receiveRequestLine();
                }
                else
                {
                    notFinished = false;
                }
            }
            else
            {
                notFinished = false;
            }
        }
        else if (expectHeaders())
        {
            const char* crlf = buf->findCRLF();
            if (crlf)
            {
                const char* colon = std::find(buf->data(), crlf, ':');
                if (colon != crlf)
                {
                    request_.addHeader(buf->data(), colon, crlf);
                }
                else
                {
                    receiveHeaders();
                    notFinished = false;
                }
                size_t len = crlf - buf->data() + 2;
                buf->retrieve(len);
            }
            else
            {
                notFinished = false;
            }
        }
    }
    return ok;
}

bool HttpParser::processRequestLine(const char* begin, const char* end)
{
    bool succeed = false;
    const char* start = begin;
    const char* space = std::find(start, end, ' '); //解析方法
    if (space != end && request_.setMethod(start, space))
    {
        start = space + 1;
        space = std::find(start, end, ' '); //解析PATH
        if (space != end)
        {
            request_.setPath(start, space);
            start = space + 1;  //HTTP/1.1
            succeed = (end - start == 8) && std::equal(start, end - 1, "HTTP/1.");
            if (succeed)
            {
                if (*(end-1) == '1')
                {
                    request_.setVersion(HttpRequest::Version::kHttp11);
                }
                else if (*(end-1) == '0')
                {
                    request_.setVersion(HttpRequest::Version::kHttp10);
                }
                else
                {
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}

