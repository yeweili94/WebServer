#include <WebServer/base/LogFile.h>
#include <WebServer/base/Timestamp.h>

#include <assert.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>

using namespace std;
using namespace ywl;

AppendFile::AppendFile(const string& filename)
    : fp_(fopen(filename.c_str(), "ae")),
      writenBytes_(0)
{
    fp_ = fopen(filename.c_str(), "ae");
    if (fp_ == NULL) {
        fprintf(stderr, "open %s failed!\n", filename.c_str());
        exit(1);
    }
    ::setbuffer(fp_, buffer_, sizeof buffer_);
}

AppendFile::~AppendFile()
{
    ::fflush(fp_);
    ::fclose(fp_);
}

void AppendFile::append(const char* data, const size_t len)
{
    int nleft = len;
    size_t nwritten;
    const char* bufp = data;
    while (nleft > 0)
    {
        nwritten = write(bufp, nleft);
        if (nwritten == 0)
        {
            int err = ::ferror(fp_);
            if (err) {
                fprintf(stderr, "AppendFile::append() failed !\n");
            }
            break;
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    writenBytes_ += len;
}

void AppendFile::flush()
{
    ::fflush(fp_);
}

size_t AppendFile::write(const char* data, size_t len)
{
    return fwrite_unlocked(data, 1, len, fp_);
}

///////////////////////////////////////////////////////////////////////

const size_t LogFile::rollSize_ = 1024 * 1024 * 1024;

LogFile::LogFile(const string& basename, int flushEveryN)
    :basename_(basename),
     flushEveryN_(flushEveryN),
     count_(0)
     // mutex_()
{
    rollFile();
}

LogFile::~LogFile()
{

}

void LogFile::append(const char* logline, int len)
{
    // MutexLockGuard lock(mutex_);
    append_unlocked(logline, len);
}

//not thread safe
void LogFile::append_unlocked(const char* data, int len)
{
    file_->append(data, len);
    ++count_;
    if (count_ >= flushEveryN_)
    {
        count_ = 0;
        flush();
    }
    if (file_->writenBytes() > rollSize_)
    {
        rollFile();
        count_ = 0;
    }
}

void LogFile::flush()
{
    // MutexLockGuard lock(mutex_);
    file_->flush();
}

void LogFile::rollFile()
{
    std::string filename = getLogFileName(basename_);
    file_.reset(new AppendFile(filename));
}

std::string LogFile::getLogFileName(const std::string& basename)
{
    std::string filename;
    filename = basename;
    char timebuf[64] = {0};

    struct timeval tv;
    ::gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    uint64_t microseconds = tv.tv_usec;
    struct tm tm_time;
    localtime_r(&seconds, &tm_time);
    snprintf(timebuf, sizeof timebuf, "%04d%02d%02d.%02d%02d%02d.%06d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, (int)microseconds);

    filename += timebuf;
    filename += ".log";
    return filename;
}






