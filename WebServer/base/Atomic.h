#ifndef WEB_SERVER_ATOMIC
#define WEB_SERVER_ATOMIC

#include <boost/noncopyable.hpp>
#include <stdint.h>

namespace ywl
{
namespace detail
{
template<typename T>
class AtomicIntegerT : boost::noncopyable
{
public:
    AtomicIntegerT() : value_(0)
    {
    }

    ~AtomicIntegerT()
    {
    }

    T get()
    {
        return __sync_val_compare_and_swap(&value_, 0, 0);
    }
    //返回更新之前的值
    T getAndAdd(T x)
    {
        return __sync_fetch_and_add(&value_, x);
    }
    //返回更新之后的值
    T addAndGet(T x)
    {
        return getAndAdd(x) + x;
    }

    T incrementAndGet()
    {
        return addAndGet(1);
    }

    T decrementAndGet()
    {
        return addAndGet(-1);
    }

    void add(T x)
    {
        getAndAdd(x);
    }

    void increment()
    {
        incrementAndGet();
    }

    void decrement()
    {
        decrementAndGet();
    }

    T getAndSet(T newValue)
    {
        return __sync_lock_test_and_set(&value_, newValue);
    }

private:
    volatile T value_;
};
}//namespace detail

using namespace detail;
typedef AtomicIntegerT<int32_t> AtomicInt32;
typedef AtomicIntegerT<int64_t> AtomicInt64;
}//namespace ywl

#endif
