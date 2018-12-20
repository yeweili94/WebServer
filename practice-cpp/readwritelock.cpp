#include <pthread.h>

//用互斥量和条件变量实现一个读写锁
class RW_Lock {
public:

    RW_Lock() : stat(0)
    {
        pthread_cond_init(&cond, NULL);
        pthread_mutex_init(&mutex, NULL);
    }

    void read_lock()
    {
        pthread_mutex_lock(&mutex);
        while (stat < 0) 
        {
            pthread_cond_wait(&cond, &mutex);
        }
        stat++;
        pthread_mutex_unlock(&mutex);
    }

    void read_unlock() 
    {
        pthread_mutex_lock(&mutex);
        if (--stat == 0) 
        {
            pthread_cond_signal(&cond);
        }
        pthread_mutex_unlock(&mutex);
    }

    void write_lock() 
    {
        pthread_mutex_lock(&mutex);
        while (stat != 0) 
        {
            pthread_cond_wait(&cond, &mutex);
        }
        stat = -1;
        pthread_mutex_unlock(&mutex);
    }

    void write_unlock() 
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_broadcast(&cond);
        stat = 0;
        pthread_mutex_unlock(&mutex);
    }

private:
    int stat; //stat == -1 writelock, stat > 0 reader number, stat == 0 free 
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};


