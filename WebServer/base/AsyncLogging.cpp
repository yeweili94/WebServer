#include <WebServer/base/AsyncLogging.h>
#include <WebServer/base/LogFile.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <memory>

using namespace ywl;

ywl::AsyncLogging::AsyncLogging(const std::string& logFileName, int flushInterval)
    : running_(false),
      basename_(logFileName),
      flushInterval_(flushInterval),
      buff_cnt_(2),
      persist_buf_(NULL),
      current_buf_(NULL),
      thread_(boost::bind(&AsyncLogging::threadFunc, this), "Logging"),
      mutex_(),
      cond_(mutex_),
      latch_(1)
{
    assert(buff_cnt > 0);
    Buffer* head = new Buffer;
    Buffer* cur = head;

    for (int i = 0; i < buff_cnt_; i++) {
        Buffer* temp = new Buffer;
        cur->next_ = temp;
        cur = cur->next_;
    }
    
    cur->next_ = head;

    //set persist buf && current_buf;
    persist_buf_ = head;
    current_buf_ = head;
}

void ywl::AsyncLogging::append(const char* data, int len) 
{
    bool wakeup = false;
    {
        MutexLockGuard lock(mutex_);
        if (current_buf_->status() == Buffer::FREE
            && current_buf_->writeableBytes() >= len)
        {
            current_buf_->append(data, len);
        }
        else
        {
            //当前缓冲区中不足以写入一条日志
            // fprintf(stderr, "current_buf foraword\n");
            current_buf_->setStatus(Buffer::FULL);
            Buffer* next_buf = current_buf_->next_;
            wakeup = true;

            if (next_buf->status() == Buffer::FULL)
            {
                assert(next_buf == persist_buf_);
                Buffer* newBuffer = new Buffer;
                newBuffer->next_ = next_buf;
                current_buf_->next_ = newBuffer;
                current_buf_ = newBuffer;
                // fprintf(stderr, "add a new buf");
            }
            else
            {
                current_buf_ = next_buf;
            }
            current_buf_->append(data, len);
        }
    }
    if (wakeup) {
        cond_.notify();
    }
}

void ywl::AsyncLogging::threadFunc()
{
    assert(running_ == true);
    latch_.countDown();
    LogFile output(basename_);
    while (running_)
    {
        {
            MutexLockGuard lock(mutex_);
            if (persist_buf_->status() == Buffer::FREE)
            {
                assert(current_buf_ == persist_buf_);
                cond_.waitForSeconds(flushInterval_);
            }

            if (persist_buf_->status() == Buffer::FREE)
            {
                assert(current_buf_ == persist_buf_);
                current_buf_->setStatus(Buffer::FULL);
                current_buf_ = current_buf_->next_;
                // fprintf(stderr, "move current_buf to persist this buff\n");
            }
        }
        //单线程持久化日志不需要加锁，因为此时的状态还是FULL
        output.append(persist_buf_->data(), persist_buf_->length());
        output.flush();
        {
            MutexLockGuard lock(mutex_);
            persist_buf_->reset();
            persist_buf_->setStatus(Buffer::FREE);
            persist_buf_ = persist_buf_->next_;
            // fprintf(stderr, "persist_buf move foraworld\n");
        }
    }
    output.flush();
}
// ywl::AsyncLogging::AsyncLogging(const std::string logFileName, int flushInterval)
//     : running_(false),
//       flushInterval_(flushInterval),
//       basename_(logFileName),
//       thread_(boost::bind(&AsyncLogging::threadFunc, this), "Logging"),
//       mutex_(),
//       cond_(mutex_),
//       latch_(1),
//       currentBuffer_(new Buffer),
//       nextBuffer_(new Buffer),
//       buffers_()
// {
//     assert(basename_.size() > 1);
//     currentBuffer_->bzero();
//     nextBuffer_->bzero();
//     buffers_.reserve(16);
// }

// void ywl::AsyncLogging::append(const char* data, int len)
// {
//     MutexLockGuard lock(mutex_);
//     if (currentBuffer_->writeableBytes() > len)
//     {
//         currentBuffer_->append(data, len);
//     }
//     else
//     {
//         buffers_.push_back(currentBuffer_);
//         currentBuffer_.reset();
//         if (nextBuffer_)
//         {
//             currentBuffer_ = std::move(nextBuffer_);
//             nextBuffer_.reset();
//         }
//         else
//         {
//             currentBuffer_.reset(new Buffer);
//         }
//         currentBuffer_->append(data, len);
//         cond_.notify();
//     }
// }

//void ywl::AsyncLogging::threadFunc()
//{
//    assert(running_ == true);
//    latch_.countDown();
//    LogFile output(basename_);

//    BufferPtr newBuffer1(new Buffer);
//    BufferPtr newBuffer2(new Buffer);
//    newBuffer1->bzero();
//    newBuffer2->bzero();

//    BufferVector bufferToWrite;
//    bufferToWrite.reserve(16);

//    while (running_)
//    {
//        assert(newBuffer1 && newBuffer1->length() == 0);
//        assert(newBuffer2 && newBuffer2->length() == 0);
//        assert(bufferToWrite.empty());

//        {
//            MutexLockGuard lock(mutex_);
//            if (buffers_.empty())
//            {
//                cond_.waitForSeconds(flushInterval_);
//            }
//            buffers_.push_back(currentBuffer_);

//            //std::move avoid to increment the count
//            currentBuffer_ = std::move(newBuffer1);
//            std::swap(bufferToWrite, buffers_);
//            //if nextBuffer_ is null it must be moved to currentBuffer_
//            //at the append function, by the way if the append is too fast
//            //acturally, buffers_.size() may be very large
//            if (!nextBuffer_)
//            {
//                //std::move avoid to increment the count
//                nextBuffer_ = std::move(newBuffer2);
//            }
//        }

//        assert(!bufferToWrite.empty());

//        for (size_t i = 0; i < bufferToWrite.size(); i++)
//        {
//            output.append(bufferToWrite[i]->data(), bufferToWrite[i]->length());
//            bufferToWrite[i]->reset();
//        }

//        if (bufferToWrite.size() > 2)
//        {
//            bufferToWrite.resize(2);
//        }

//        //newBuffer1 和 newBuffer2只会在后端写入线程被调用,不必要加锁
//        if (!newBuffer1)
//        {
//            assert(!bufferToWrite.empty());
//            newBuffer1 = bufferToWrite.back();
//            bufferToWrite.pop_back();
//        }

//        if (!newBuffer2)
//        {
//            assert(!bufferToWrite.empty());
//            newBuffer2 = bufferToWrite.back();
//            bufferToWrite.pop_back();
//        }

//        bufferToWrite.clear();
//        output.flush();
//    }
//    output.flush();
//}

