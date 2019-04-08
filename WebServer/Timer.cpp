#include <WebServer/Timer.h>

#include <WebServer/base/Logging.h>
#include <WebServer/EventLoop.h>

#include <boost/bind.hpp>
#include <sys/timerfd.h>
#include <stdio.h>

namespace ywl
{
namespace net
{
std::atomic<uint64_t> Timer::timerId_;

TimerManager::TimerManager(EventLoop* loop)
    : loop_(loop)
{
}

TimerManager::~TimerManager()
{
}

void TimerManager::addTimer(const TimerCallback& cb, Timestamp when, double interval) {
    std::shared_ptr<Timer> newTimer(new Timer(cb, when, interval));
    // bool earliestChange = earliestChanged(newTimer);
    timerQueue_.push(newTimer);
    // if (earliestChange) {
    //     loop_->wakeup();
    // }
}

void TimerManager::handleExpired()
{
    Timestamp now(Timestamp::now());
    // fprintf(stderr, "now time is %d\n", now.msecSecondsSinceEpoch());
    while (!timerQueue_.empty()) {
        std::shared_ptr<Timer> top = timerQueue_.top();
        if (top->msecSecondsSinceEpoch() <= now.msecSecondsSinceEpoch()) {
            timerQueue_.pop();
            if (top->repeat() && top->isValid()) {
                top->run();
                top->restart(now);
                timerQueue_.push(top);
            }
        } else {
            break;
        }
    }
}

int TimerManager::nextExpired()
{
    if (timerQueue_.empty()) {
        return -1;
    }
    Timestamp now(Timestamp::now());
    int diff = timerQueue_.top()->msecSecondsSinceEpoch() - now.msecSecondsSinceEpoch();
    //return 0 表示时间偏移或者定时器设置太短，已经有定时器超时了
    //所以epoll直接返回处理超时事件
    return diff > 0 ? diff : 0;
}

bool TimerManager::earliestChanged(const std::shared_ptr<Timer>& timerPtr)
{
    if (timerQueue_.empty()) return true;
    std::shared_ptr<Timer> top = timerQueue_.top();
    if (timerPtr->msecSecondsSinceEpoch() <= top->msecSecondsSinceEpoch()) return true;
    return false;
}

}//namspace net
}//namespace ywl
