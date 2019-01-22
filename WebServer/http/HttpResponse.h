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
    std::map<std::string, std::string> headers_;    //header�б�

    HttpStatusCode statusCode_; //״̬��Ӧ��

    std::string statusMessage_; //״̬��Ӧ���Ӧ���ı���Ϣ
    bool closeConnection_;  //�Ƿ�ر�����
    std::string body_;  //ʵ��
};

}
}

#endif
