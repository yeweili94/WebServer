#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include <boost/function.hpp>

class Thread {
public:
    using ThreadFunc = boost::function<void ()>;
    explicit Thread(const ThreadFunc& func);
    ~Thread();
    void Start();
    void Join();
    void SetAutoDelete(bool autodelete);
private:
    void Run();
    static void* ThreadRoutine(void* arg);
    pthread_t threadId_;
    bool autoDelete_;
    ThreadFunc func_;
};

Thread::Thread(const ThreadFunc& func) : autoDelete_(false), func_(func) {}

Thread::~Thread() {
    std::cout << "Thread deconstructing...." << std::endl;
}

void Thread::Start() {
    pthread_create(&threadId_, NULL, ThreadRoutine, this);
}

void Thread::Join() {
    pthread_join(threadId_, NULL);
}

void Thread::Run() {
    func_();
}

void Thread::SetAutoDelete(bool autodelete) {
    autoDelete_ = autodelete;
}

void* Thread::ThreadRoutine(void* arg) {
    Thread* thread = static_cast<Thread*> (arg);
    thread->Run();
    if (thread->autoDelete_) {
        delete thread;
    }
    return NULL;
}

void func() {
    std::cout << "this is test function " << std::endl;
}
int main() {
    Thread* t = new Thread(func);
    t->SetAutoDelete(true);
    t->Start();
    t->Join();
    sleep(1);
}
