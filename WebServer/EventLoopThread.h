#ifndef WEBSERVER_EVENTLOOP_THREAD_H
#define WEBSERVER_EVENTLOOP_THREAD_H

#include <WebServer/base/Thread.h>
#include <WebServer/base/Mutex.h>
#include <WebServer/base/Condition.h>

#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace ywl
{
namespace net
{

class EventLoop;
using ThreadInitCallback = boost::function<void(EventLoop*)>;

class EventLoopThread : boost::noncopyable
{
public:
    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback());
    ~EventLoopThread();
    EventLoop* startLoop();
private:
    void threadFunc();  //Ïß³Ìº¯Êý

    EventLoop* loop_;
    bool exiting_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
    ThreadInitCallback callback_;
};

////////////////////////////////////////////////////////////////////////
class EventLoopThreadPool : boost::noncopyable
{
public:
    EventLoopThreadPool(EventLoop* mainLoop);
    ~EventLoopThreadPool();
    void setThreadNum(int numThreads) {
        numThreads_ = numThreads;
    }
    void start(const ThreadInitCallback& cb = ThreadInitCallback());
    EventLoop* getNextLoop();

private:
    EventLoop *mainLoop_;
    int numThreads_;
    int next_;
    bool started_;
    boost::ptr_vector<EventLoopThread> threads_;
    std::vector<EventLoop*> loops_;
};


}
}
#endif
