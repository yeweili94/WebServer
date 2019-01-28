#ifndef WEB_SERVER_ASYNCLOGGING_H
#define WEB_SERVER_ASYNCLOGGING_H

#include <WebServer/base/LogStream.h>
#include <WebServer/base/Thread.h>
#include <WebServer/base/CountDownLatch.h>

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

namespace ywl
{

class AsyncLogging : boost::noncopyable
{
public:
    AsyncLogging(const std::string basename, int flushInterval = 2);
    ~AsyncLogging()
    {
        if (running_) {
            stop();
        }
    }

    void append(const char* data, int len);

    void start()
    {
        running_ = true;
        thread_.start();
        latch_.wait();
    }

    void stop()
    {
        running_ = false;
        cond_.notify();
        thread_.join();
    }

private:
    void threadFunc();

    typedef FixedBuffer<KLargeBuffer> Buffer;
    typedef std::vector<boost::shared_ptr<Buffer>> BufferVector;
    typedef boost::shared_ptr<Buffer> BufferPtr;

    bool running_;
    const int flushInterval_;
    std::string basename_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
    CountDownLatch latch_;

    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
};

}//ywl
#endif

