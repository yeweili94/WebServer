#include <WebServer/base/DynamicThreadPool.h>

namespace ywl
{

DynamicThreadPool::DynamicThread::DynamicThread(DynamicThreadPool* pool)
    : pool_(pool),
      thread_(boost::bind(&DynamicThreadPool::DynamicThread::ThreadFunc, this), "thread") {
    thread_.start();
}

DynamicThreadPool::DynamicThread::~DynamicThread()
{
    thread_.join();
}
    
void DynamicThreadPool::DynamicThread::ThreadFunc()
{
    pool_->ThreadFunc();
    //Now that we have killed ourselves, we should reduce thread count
    std::unique_lock<std::mutex> lock(pool_->mutex_);
    pool_->loop_threads_--;
    //Move to dead list
    pool_->dead_threads_.push_back(this);

    if ((pool_->shutdown_) && (pool_->loop_threads_ == 0)) {
        pool_->shutdown_cond_.notify_one();
    }
}

void DynamicThreadPool::ThreadFunc() {
    for ( ; ; ) {
        //wait until work is available or shutdown
        std::unique_lock<std::mutex> lock(mutex_);
        if (!shutdown_ && callbacks_.empty()) {
            // if (threads_waiting_ > reserve_threads_) {
                // break;
            // }
            threads_waiting_++;
            cond_.wait(lock);
            threads_waiting_--;
        }

        if (!callbacks_.empty()) {
            auto cb = callbacks_.front();
            callbacks_.pop();
            lock.unlock();
            cb();
        } else if (shutdown_) {
            break;
        }
    }
}


DynamicThreadPool::DynamicThreadPool(int reserve_threads)
    : shutdown_(false),
      reserve_threads_(reserve_threads),
      loop_threads_(0),
      threads_waiting_(0) {
    for (int i = 0; i < reserve_threads_; i++) {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_threads_++;
        new DynamicThread(this);
    }
}


void DynamicThreadPool::ReapThreads(std::list<DynamicThread*>* tlist) {
    for (auto t = tlist->begin(); t != tlist->end(); t = tlist->erase(t)) {
        delete *t;
    }
}

DynamicThreadPool::~DynamicThreadPool() {
    std::unique_lock<std::mutex> lock(mutex_);
    shutdown_ = true;
    cond_.notify_all();
    while (loop_threads_ != 0) {
        shutdown_cond_.wait(lock);
    }
    ReapThreads(&dead_threads_);
}

void DynamicThreadPool::Add(const boost::function<void()>& callback) {
    std::unique_lock<std::mutex> lock(mutex_);
    //Add works to thre callback list
    callbacks_.push(callback);
    //increse pool size or notify as needed
    if ((threads_waiting_ == 0) && (loop_threads_ < 2 * reserve_threads_)) {
        loop_threads_++;
        new DynamicThread(this);
    } else if (threads_waiting_ != 0){
        cond_.notify_one();
    }
    //also use this chance to harvest dead threads
    if (!dead_threads_.empty()) {
        std::list<DynamicThread*> tmplist;
        dead_threads_.swap(tmplist);
        lock.unlock();
        ReapThreads(&tmplist);
    }
}


}   // namespace ywl
