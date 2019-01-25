#include <WebServer/base/Timestamp.h>
#include <map>
#include <vector>
#include <atomic>
#include <boost/bind.hpp>
#include <boost/function.hpp>
namespace ywl
{
namespace net
{
namespace test
{
typedef boost::function<void()> TimerCallback;

class timer
{
public:
    timer(const TimerCallback& cb, Timestamp when, double interval)
        : interval_(interval),
          expired_(when),
          repeated_(interval > 0),
          timerCallback_(cb),
          sequence_(numCreated_.increment())
    {}
    ~timer();
    void run() const
    {
        timerCallback_();
    }
    bool repeated()
    {
        return repeated_;
    }
    size_t interval()
    {
        return interval_;
    }
private:
    size_t interval_;
    Timestamp expired_;
    bool repeated_;
    TimerCallback timerCallback_;
    const int64_t sequence_;
    static std::atomic<int64_t> numCreated_;
};
}
}
}
