#ifndef MUDUO_STUDY_SINGLETON
#define MUDUO_STUDY_SINGLETON

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
        pthread_once(&ponce_, &Singleton::init);
        return *value_;
    }
private:
    Singleton() = delete;
    ~Singleton() = delete;
    static void init()
    {
        value_ = new T();
        ::atexit(Singleton::destroy);
    }
    static void destroy()
    {
        typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
        delete value_;
    }
private:
    static T* value_;
    static pthread_once_t ponce_;
};

template<typename T>
T* Singleton<T>::value_ = NULL;

template<typename T>
pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

}//ywl
#endif
