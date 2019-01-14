#include <pthread.h>
#include <WebServer/base/CurrentThread.h>
#include <assert.h>
using namespace ywl;

class MutexLock
{
public:
    MutexLock();
    ~MutexLock();

    void lock();
    void unlock();
    pthread_mutex_t* getMutex(); 

private:
    pthread_t holder_;
    pthread_mutex_t mutex_;
};

MutexLock::MutexLock()
    : holder_(0)
{
    mutex_ = PTHREAD_MUTEX_INITIALIZER;
}

void MutexLock::lock()
{
    pthread_mutex_lock(&mutex_);
    holder_ = CurrentThread::tid();
}

void MutexLock::unlock()
{
    holder_ = 0;
    pthread_mutex_unlock(&mutex_);
}

pthread_mutex_t* getMutex()
{
    return &mutex_;
}
MutexLock::~MutexLock()
{
    assert(holder_ == 0);
    pthread_mutex_destroy(&mutex_);
}

class Condition
{
public:
    Condition(const MutexLock& mutex);
    ~Condition();

private:
    Condition(const Condition& cond) = delete;
    void operator=(const Condition& cond) = delete;

    pthread_cond_t cond_;
    pthread_mutex_t mutex_;
};
