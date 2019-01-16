#ifndef WEB_SERVER_LOG_FILE_H
#define WEB_SERVER_LOG_FILE_H

#include <WebServer/base/Mutex.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include <string>
#include <memory>

namespace ywl
{

class AppendFile : public boost::noncopyable
{
public:
    explicit AppendFile(const std::string& filename);
    ~AppendFile();

    void append(const char* log, const size_t len);
    void flush();
    size_t writenBytes() { return writenBytes_; }

private:
    size_t write(const char* log, size_t len);
    FILE* fp_;
    size_t writenBytes_;
    char buffer_[64*1024];
};

//////////////////////////////////////////////////////////////////////
class LogFile : public boost::noncopyable
{
public:
    LogFile(const std::string& basename, int flushEveryN = 1024);
    ~LogFile();

    void append(const char* logline, int len);
    void flush();

private:
    void append_unlocked(const char* logline, int len);
    void rollFile();
    std::string getLogFileName(const std::string& basename);

    const std::string basename_;
    const int flushEveryN_;
    static const size_t rollSize_;

    int count_;
    // MutexLock mutex_;
    boost::scoped_ptr<AppendFile> file_;
};

}
#endif
