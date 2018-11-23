#include <pthread.h>

//用互斥量和条件变量实现一个读写锁
class rw_lock {
public:
    rw_lock() : stat(0)
    {
        pthread_cond_init(&cond, NULL);
        pthread_mutex_init(&mutex, NULL);
    }

    void r_lock() {
        pthread_mutex_lock(&mutex);
        while (stat < 0) {
            pthread_cond_wait(&cond, &mutex);
        }
        stat++;
        pthread_mutex_unlock(&mutex);
    }

    void r_unlock() {
        pthread_mutex_lock(&mutex);
        if (--stat == 0) {
            pthread_cond_signal(&cond);
        }
        pthread_mutex_unlock(&mutex);
    }

    void w_lock() {
        pthread_mutex_lock(&mutex);
        while (stat != 0) {
            pthread_cond_wait(&cond, &mutex);
        }
        stat = -1;
        pthread_mutex_unlock(&mutex);
    }

    void w_unlock() {
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
