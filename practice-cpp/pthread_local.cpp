#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
using namespace std;

pthread_key_t key;  
pthread_once_t once_control = PTHREAD_ONCE_INIT;  
typedef struct Tsd  
{  
    pthread_t tid;  
    char *str;  
} tsd_t;  
  
//线程特定数据销毁函数,  
//用来销毁每个线程所指向的实际数据  
void destructor_function(void *value)  
{  
    free(value);  
    std::cout << "destructor ..." << endl;  
}  
  
//初始化函数, 将对key的初始化放入该函数中,  
//可以保证inti_routine函数只运行一次  
void init_routine()  
{  
    pthread_key_create(&key, destructor_function);  
    std::cout << "init..." << endl;  
}  
  
void *thread_routine(void *args)  
{  
    pthread_once(&once_control, init_routine);  
  
    //设置线程特定数据  
    tsd_t *value = (tsd_t *)malloc(sizeof(tsd_t));  
    value->tid = pthread_self();  
    value->str = (char *)args;  
    pthread_setspecific(key, value);  
    printf("%s setspecific, address: %p\n", (char *)args, value);  
  
    //获取线程特定数据  
    value = (tsd_t *)pthread_getspecific(key);  
    printf("tid: 0x%x, str = %s\n", (unsigned int)value->tid, value->str);  
    sleep(1);  
  
    //再次获取线程特定数据  
    value = (tsd_t *)pthread_getspecific(key);  
    printf("tid: 0x%x, str = %s\n", (unsigned int)value->tid, value->str);  
  
    pthread_exit(NULL);  
}  
  
int main()  
{  
    pthread_t tid1, tid2;  
    pthread_create(&tid1, NULL, thread_routine, (void *)"thread1");  
    pthread_create(&tid2, NULL, thread_routine, (void *)"thread2");  
  
    pthread_join(tid1, NULL);  
    pthread_join(tid2, NULL);  
    pthread_key_delete(key);  
  
    return 0;  
}  

