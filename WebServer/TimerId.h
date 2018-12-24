#ifndef WEB_NET_TIMERID_H
#define WEB_NET_TIMERID_H

#include "Timer.h"

namespace ywl
{
namespace net
{

class TimerId
{
public:
    TimerId() : timer_(NULL), sequence_(0)
    {

    }

    TimerId(Timer* timer, int64_t seq)
        : timer_(timer),
          sequence_(seq)
    {

    }

    friend class TimerQueue;

private:
    Timer* timer_;
    int64_t sequence_;
};

}
}

#endif
