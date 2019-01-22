#ifndef WEBSERVER_HTTPREQUEST_H
#define WEBSERVER_HTTPREQUEST_H

#include <WebServer/base/Timestamp.h>
#include <WebServer/base/Timestamp.h>

#include <string>
#include <map>

#include <assert.h>
#include <stdio.h>

namespace ywl
{
namespace net
{

class HttpRequest
{
public:
    enum class Method
    {
        kInvalid, kGet, kPost, kHead, kPut, kDelete
    };

    enum Version
    {
        kUnkonwn, kHttp10, kHttp11
    };

    HttpRequest()
        : method_(Method::kInvalid),
          version_(Version::kUnkonwn)
    {

    }

    void setVersion(Version v)
    {
        version_ = v;
    }

    Version getVersion() const
    {
        return version_;
    }

    bool setMethod(const char* start, const char* end)
    {
        assert(method_ == kInvalid);
        std::string method(start, end);
        if (method == "GET")
        {
            method_ = Method::kGet;
        }
        else if (method == "POST")
        {
            method_ = Method::kPost;
        }
        else if (method == "HEAD")
        {
            method_ = Method::kHead;
        }
        else if (method == "PUT")
        {
            method_ = Method::kPut;
        }
        else if (method == "DELETE")
        {
            method_ = Method::kDelete;
        }
        else
        {
            method_ = Method::kInvalid;
        }
        return method_ != Method::kInvalid;
    }

    Method method() const
    {
        return method_;
    }

    const char* methodString() const 
    {
        const char* result = "UNKNOWN";
        switch(method_)
        {
            case Method::kGet:
                result = "GET";
                break;
            case Method::kPost:
                result = "POST";
                break;
            case Method::kPut:
                result = "PUT";
                break;
            case Method::kDelete:
                result = "DELETE";
                break;
            case Method::kHead:
                result = "HEAD";
                break;
            default:
                break;
        }
        return result;
    }

    void setPath(const char* start, const char* end)
    {
        path_.assign(start, end);
    }

    const std::string& path() const 
    {
        return path_;
    }

    void setReceiveTime(Timestamp t)
    {
        receiveTime_ = t;
    }

    Timestamp receiveTime() const 
    {
        return receiveTime_;
    }

    void addHeader(const char* start, const char* colon, const char* end)
    {
        std::string field(start, colon);    //×Ö¶Î
        ++colon;
        while (colon < end && isspace(*colon))
        {
            ++colon;
        }
        while (end > colon && *end == ' ') {
            end--;
        }
        std::string value(colon, end);
        headers_[field] = value;
    }

    std::string getHeader(const std::string& field) const {
        std::string result;
        auto it = headers_.find(field);
        if (it != headers_.end())
        {
            result = it->second;
        }
        return result;
    }

    const std::map<std::string, std::string>& headers() const
    {
        return headers_;
    }

    void swap(HttpRequest& that)
    {
        std::swap(method_, that.method_);
        std::swap(path_, that.path_);
        std::swap(receiveTime_, that.receiveTime_);
        std::swap(headers_, that.headers_);
        std::swap(version_, that.version_);
    }

private:
    Method method_;
    Version version_;
    std::string path_;
    Timestamp receiveTime_;
    std::map<std::string, std::string> headers_;
};

}
}

#endif
