#include "Logging.h"
#include "Thread.h"
#include "CurrentThread.h"
#include "AsyncLogging.h"

#include <assert.h>
#include <iostream>
#include <time.h>
#include <sys/time.h>


static pthread_once_t ponce_ = PTHREAD_ONCE_INIT;
//singleton
static ywl::AsyncLogging *AsyncLogger_;

//默认初始化log文件名
std::string ywl::Logger::logFileName_ = "./ywl_WebServer";

static void once_init()
{
    AsyncLogger_ = new ywl::AsyncLogging(ywl::Logger::getLogFileName());
    AsyncLogger_->start();
}

static void output(const char* msg, int len)
{
    pthread_once(&ponce_, once_init);
    AsyncLogger_->append(msg, len);
}

namespace ywl
{
__thread char t_time[32];

Logger::Impl::Impl(const char* filename, int line)
    : stream_(),
      line_(line),
      logPosFileName_(filename),
      time_(Timestamp::now())
{
    formatTime();
}

void Logger::Impl::formatTime()
{
    int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / (1000*1000));

    struct tm tm_time;
    ::gmtime_r(&seconds, &tm_time);

    int len = snprintf(t_time, sizeof(t_time), "%4d-%02d-%02d %02d:%02d:%02d\n",
                       tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                       tm_time.tm_hour+8, tm_time.tm_min, tm_time.tm_sec);
    assert(len == 20); (void)len;
    stream_ << t_time;
}

//初始化LOG所在文件和行数,便于查找bug
Logger::Logger(const char* fileName, int line, int level)
    : impl_(fileName, line),
      level_(level)
{

}

Logger::~Logger()
{
    impl_.stream_ << " -- " << impl_.logPosFileName_ << ':' << impl_.line_ << '\n';
    const LogStream::Buffer& buf(stream().buffer());
    output(buf.data(), buf.length());
    if (level_ == 1)
    {
        sleep(3);
        abort();
    }
}

}
