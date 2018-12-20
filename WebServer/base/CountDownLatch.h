#ifndef WEB_SERVER_COUNTDOWNlATCH_H
#define WEB_SERVER_COUNTDOWNlATCH_H

#include "Condition.h"
#include "Mutex.h"
#include <boost/noncopyable.hpp>

namespace ywl
{

class CountDownLatch : boost::noncopyable
{
public:
    explicit CountDownLatch(int count);

    void wait();

    void countDown();

    int getCount() const;

private:
    mutable MutexLock mutex_;
    Condition condition_;
    int count_;
};

}
#endif
