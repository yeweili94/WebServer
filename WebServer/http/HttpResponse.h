#ifndef WEBSERVER_HTTPRESPONSE_H
#define WEBSERVER_HTTPRESPONSE_H

#include <string>
#include <map>

namespace ywl
{
namespace net
{

class Buffer;
class HttpResponse
{
public:
    enum HttpStatusCode
    {
        kUnknow,
        k200OK = 200,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k404NotFound = 404,
    };

    explicit HttpResponse(bool close)
        : statusCode_(kUnknow),
          closeConnection_(close)
    {

    }

    void setStatusCode(HttpStatusCode code)
    {
        statusCode_ = code;
    }

    void setStatusMessage(const std::string& message)
    {
        statusMessage_ = message;
    }
    
    void setCloseConnection(bool on)
    {
        closeConnection_ = on;
    }

    bool iscloseConnection() const 
    {
        return closeConnection_;
    }

    void setContentType(const std::string& contentType)
    {
        addHeader("Content-Type", contentType);
    }

    void addHeader(const std::string& key, const std::string& value)
    {
        headers_[key] = value;
    }

    void setBody(const std::string& body)
    {
        body_ = body;
    }

    void appendToBuffer(Buffer* output) const;


private:
    std::map<std::string, std::string> headers_;    //header列表

    HttpStatusCode statusCode_; //状态响应码

    std::string statusMessage_; //状态相应码对应的文本信息
    bool closeConnection_;  //是否关闭连接
    std::string body_;  //实体
};

}
}

#endif
