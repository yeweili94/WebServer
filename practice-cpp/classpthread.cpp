//面向对象的thread类的封装，演示面向对象c++中回调函数的用法，muduo中使用的是基于对象的封装。
//把纯虚函数Run()封装为private是必须的，不能暴露Run方法给用户使用，不然这个run方法是在主线程中进行的。
//线程对象的生命周期和线程的生命周期是不同的
//如何让线程执行完毕，线程自动销毁？ : 需要使用动态创建对象 ThreadTests* t = new ThreadTest();
#include <pthread.h>
#include <iostream>
#include <unistd.h>

class Thread{
public:
    explicit Thread(bool autodelete) : autodelete_ (autodelete){}
    virtual ~Thread() {}
    void Start();
    void Join();
    //函数执行入口设为static,不能将Run方法直接传递给pthread_create(),因为Run方法作为成员方法有一个参数是this
    //设置ThreadRoutine静态方法作为适配
    static void* ThreadRoutine(void *arg);

    void SetAutoDelete(bool autodelete);
private:
    virtual void Run() = 0;
    pthread_t threadId_;
    bool autodelete_;
};


void Thread::Start() {
    pthread_create(&threadId_, NULL, ThreadRoutine, this);
}

void Thread::Join() {
    pthread_join(threadId_, NULL);
}

void* Thread::ThreadRoutine(void *arg) {
    Thread* thread = static_cast<Thread*>(arg);
    thread->Run();
    if (thread->autodelete_) {
        delete thread;
    }
    return NULL;
}

void Thread::SetAutoDelete(bool autodelete) {
    autodelete_ = autodelete;
}
///////////////////////////////////////////////

class ThreadTests : public Thread {
public:
    ThreadTests(bool autodelete) : Thread(autodelete){
        std::cout << "test thread constructing..." << std::endl;
    }
    ~ThreadTests(){
        std::cout << "test thread deconstructing...." << std::endl;
    }

private:
    void Run() override {
        std::cout << "the Run function running..." << std::endl;
        sleep(1);
    }

};

int main() {
    ThreadTests *t = new ThreadTests(false);
    t->Start();
    t->Join();
    sleep(2);
    delete t;
    return 0;
}
