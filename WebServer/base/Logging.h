#ifndef WEB_SERVER_LOGGING_H
#define WEB_SERVER_LOGGING_H

#include "LogStream.h"

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <string>

namespace ywl
{
class AsyncLogging;

class Logger
{
public:
    Logger(const char* fileName, int line);
    ~Logger();
    LogStream& stream() { return impl_.stream_; }

    static void setLogFileName(std::string fileName)
    {
        logFileName_ = fileName;
    }

    static std::string getLogFileName()
    {
        return logFileName_;
    }

private:
    class Impl
    {
    public:
        Impl(const char* fileName, int line);
        void formatTime();

        LogStream stream_;
        int line_;
        std::string basename_;
    private:
    };

    Impl impl_;
    static std::string logFileName_;
};

}
#define LOG ywl::Logger(__FILE__, __LINE__).stream()

#endif
