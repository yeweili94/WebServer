#ifndef WEB_SERVER_THREAD_H
#define WEB_SERVER_THREAD_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <atomic>
#include <pthread.h>

namespace ywl
{
class Thread : public boost::noncopyable
{
public:
    using ThreadFunc = boost::function<void ()>;
    
    explicit Thread(const ThreadFunc&, const std::string& name = std::string());
    ~Thread();

    void start();
    int join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    static int numCreated() { return numCreated_.fetch_add(0); }
private:
    static void* startThread(void* arg);
    void runInThread();

    bool started_;
    pthread_t pthreadId_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;

    static std::atomic<int32_t> numCreated_;
};


}//ywl
#endif

