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
    void threadFunc();  //线程函数

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
    EventLoop *mainLoop_;   //acceptor所属的eventLoop
    int numThreads_;    //线程数
    int next_;  //新连接到来，所选则的EventLoop对象下标
    bool started_;
    boost::ptr_vector<EventLoopThread> threads_;    //IO线程列表
    std::vector<EventLoop*> loops_; //EventLoop列表
};


}
}
#endif
