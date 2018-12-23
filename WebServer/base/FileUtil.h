#ifndef WEB_SERVER_FILEUTIL_H
#define WEB_SERVER_FILEUTIL_H

#include <boost/noncopyable.hpp>

#include <string>

namespace ywl
{

class AppendFile : public boost::noncopyable
{
public:
    explicit AppendFile(const std::string& filename);
    ~AppendFile();

    void append(const char* log, const size_t len);
    void flush();

private:
    size_t write(const char* log, size_t len);
    FILE* fp_;
    char buffer_[64*1024];
};

}
#endif
