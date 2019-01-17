#include <WebServer/base/AsyncLogging.h>
#include <WebServer/base/LogFile.h>

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <memory>

ywl::AsyncLogging::AsyncLogging(const std::string logFileName, int flushInterval)
    : running_(false),
      flushInterval_(flushInterval),
      basename_(logFileName),
      thread_(boost::bind(&AsyncLogging::threadFunc, this), "Logging"),
      mutex_(),
      cond_(mutex_),
      latch_(1),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_()
{
    assert(basename_.size() > 1);
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}

void ywl::AsyncLogging::append(const char* logline, int len)
{
    MutexLockGuard lock(mutex_);
    if (currentBuffer_->writeableBytes() > len)
    {
        currentBuffer_->append(logline, len);
    }
    else
    {
        buffers_.push_back(currentBuffer_);
        currentBuffer_.reset();
        if (nextBuffer_)
        {
            currentBuffer_ = std::move(nextBuffer_);
        }
        else
        {
            currentBuffer_.reset(new Buffer);
        }
        currentBuffer_->append(logline, len);
        cond_.notify();
    }
}

void ywl::AsyncLogging::threadFunc()
{
    assert(running_ == true);
    latch_.countDown();
    LogFile output(basename_);

    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();

    BufferVector bufferToWrite;
    bufferToWrite.reserve(16);

    while (running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(bufferToWrite.empty());

        {
            MutexLockGuard lock(mutex_);
            if (buffers_.empty())
            {
                cond_.waitForSeconds(flushInterval_);
            }
            buffers_.push_back(currentBuffer_);

            //std::move avoid to increment the count
            currentBuffer_ = std::move(newBuffer1);
            std::swap(bufferToWrite, buffers_);
            //if nextBuffer_ is null it must be moved to currentBuffer_
            //at the append function, by the way if the append is too fast
            //acturally, buffers_.size() may be very large
            if (!nextBuffer_)
            {
                //std::move avoid to increment the count
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        assert(!bufferToWrite.empty());

        // if (bufferToWrite.size() > 25)
        // {
            // bufferToWrite.resize(2);
        // }

        for (size_t i = 0; i < bufferToWrite.size(); i++)
        {
            output.append(bufferToWrite[i]->data(), bufferToWrite[i]->length());
            bufferToWrite[i]->reset();
        }

        if (bufferToWrite.size() > 2)
        {
            bufferToWrite.resize(2);
        }

        //newBuffer1 和 newBuffer2只会在后端写入线程被调用,不必要加锁
        if (!newBuffer1)
        {
            assert(!bufferToWrite.empty());
            newBuffer1 = bufferToWrite.back();
            bufferToWrite.pop_back();
        }

        if (!newBuffer2)
        {
            assert(!bufferToWrite.empty());
            newBuffer2 = bufferToWrite.back();
            bufferToWrite.pop_back();
        }

        bufferToWrite.clear();
        output.flush();
    }
    output.flush();
}

