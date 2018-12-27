#ifndef WEB_SERVER_LOG_FILE_H
#define WEB_SERVER_LOG_FILE_H

#include <WebServer/base/FileUtil.h>
#include <WebServer/base/Mutex.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include <string>
#include <memory>

namespace ywl
{

class LogFile : public boost::noncopyable
{
public:
    LogFile(const std::string& basename, int flushEveryN = 1024);
    ~LogFile();

    void append(const char* logline, int len);
    void flush();
    bool rollFile();

private:
    void append_unlocked(const char* logline, int len);

    const std::string basename_;
    const int flushEveryN_;

    int count_;
    MutexLock mutex_;
    boost::scoped_ptr<AppendFile> file_;
};

}
#endif
