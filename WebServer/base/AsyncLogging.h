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
    AsyncLogging(const std::string& basename, int flushInterval = 2);
    ~AsyncLogging()
    {
        if (running_) {
            stop();
        }
        
        Buffer* slow = persist_buf_;
        Buffer* fast = slow->next_;
        while (fast != slow)
        {
            Buffer* next = fast->next_;
            delete fast;
            fast = next;
        }
        delete slow;
        printf("current buf count is %d\n", buff_cnt_);
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
    bool running_;

    std::string basename_;
    const int flushInterval_;

    typedef FixedBuffer<KLargeBuffer> Buffer;
    int buff_cnt_;
    Buffer* persist_buf_;
    Buffer* current_buf_;
    // typedef std::vector<boost::shared_ptr<Buffer>> BufferVector;
    // typedef boost::shared_ptr<Buffer> BufferPtr;

    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
    CountDownLatch latch_;

    // BufferPtr currentBuffer_;
    // BufferPtr nextBuffer_;
    // BufferVector buffers_;
};

}//ywl
#endif

