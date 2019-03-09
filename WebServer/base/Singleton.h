#ifndef WEB_SERVER_SINGLETON
#define WEB_SERVER_SINGLETON

#include <WebServer/base/CurrentThread.h>

#include <boost/noncopyable.hpp>

#include <pthread.h>
#include <stdlib.h> //::atexit

namespace ywl
{
template<typename T>
class Singleton : public boost::noncopyable
{
public:
    static T& instance()
    {
__asm__ __volatile__("":::"memory");
        if (value_ == NULL) {
            pthread_mutex_lock(&mutex_);
            if (value_ == NULL) {
                value_ = new T();
                ::atexit(Singleton::destroy);
            }
__asm__ __volatile__("":::"memory");
            pthread_mutex_unlock(&mutex_);
        }
        return *value_;
    }
private:
    Singleton() = delete;
    ~Singleton() = delete;
    static void destroy()
    {
        delete value_;
    }

private:
    static T* value_;
    static pthread_mutex_t mutex_;
};

template<typename T>
T* Singleton<T>::value_ = NULL;

template<typename T>
pthread_mutex_t Singleton<T>::mutex_ = PTHREAD_MUTEX_INITIALIZER;

}//ywl
#endif
