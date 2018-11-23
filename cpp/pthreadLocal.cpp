#include<pthread.h>
#include<unistd.h>
#include<stdio.h>
#include<sys/types.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#define ERR_EXIT(m) do{perror(m);exit(EXIT_FAILURE);}while(0)
pthread_key_t key_tsd;
//pthread_once_t once_control = PTHREAD_ONCE_INIT;

struct tsd
{
   pthread_t tid;
   char *str;
};

void destroy_routine(void *value)
{
    printf("destroy\n");  
    free(value);
}

//void once_routine(void)
//{
//    pthread_key_create(&key_tsd,destroy_routine);   
//    printf("key init....\n");
//}

void* thread_routine(void *arg)
{
    tsd *value=(tsd*)malloc(sizeof(tsd));
    value->tid=pthread_self();
    value->str=(char*)arg;
    pthread_setspecific(key_tsd, value);
    printf("%s setspecific %p\n",(char*)arg,value);
    value=(tsd*)pthread_getspecific(key_tsd);
    printf("tid=%x,str=%s\n",(int)value->tid,value->str);
    sleep(2);
    value=(tsd*)pthread_getspecific(key_tsd);   
    printf("tid=%x,str=%s\n",(int)value->tid,value->str);
    return NULL;
}

int main()
{
    pthread_key_create(&key_tsd,destroy_routine);
    pthread_t tid1,tid2;
    char ch1[]="thread1";
    char ch2[]="thread2";
    pthread_create(&tid1,NULL,thread_routine,(void*)ch1);
    pthread_create(&tid2,NULL,thread_routine,(void*)ch2);
    pthread_join(tid1,NULL);
    pthread_join(tid2,NULL);
    pthread_key_delete(key_tsd);
    return 0;
}
