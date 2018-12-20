#ifndef MUDUO_STUDY_THREAD_H
#define MUDUO_STUDY_THREAD_H

#include "atomic.h"
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace ywl
{
using namespace detail;
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

    static int numCreated() { return numCreated_.get(); }
private:
    static void* startThread(void* arg);
    void runInThread();

    bool started_;
    pthread_t pthreadId_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;

    static AtomicInt32 numCreated_;
};


}//ywl
#endif

