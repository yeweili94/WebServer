#ifndef WEBSERVER_MEMORY_POOL_H
#define WEBSERVER_MEMORY_POOL_H

#include <WebServer/base/Mutex.h>

#include <boost/noncopyable.hpp>
#include <memory>
#include <mutex>

#include <assert.h>
#include <stdio.h>

namespace ywl
{

template<size_t ObjectSize, size_t NumofObjects = 40960>
class MemoryPool : boost::noncopyable
{
public:
    explicit MemoryPool() {
        freeNodeHeader_ = NULL;
        memBlockHeader_ = NULL;
    }

    ~MemoryPool() {
        MemBlock* ptr;
        while (memBlockHeader_) {
            ptr = memBlockHeader_->pNext_;
            delete memBlockHeader_;
            memBlockHeader_ = ptr;
        }
    }

    void* malloc();
    void free(void*);

private:
    struct FreeNode
    {
        FreeNode* pNext_;
        char data[ObjectSize];
    };

    struct MemBlock
    {
        MemBlock* pNext_;
        FreeNode data[NumofObjects];
    };

    FreeNode* freeNodeHeader_;
    MemBlock* memBlockHeader_;
    std::mutex mutex_;
};

template<size_t ObjectSize, size_t NumofObjects>
void* MemoryPool<ObjectSize, NumofObjects>::malloc()
{
    std::unique_lock<std::mutex> lock(mutex_);
    {
        if (freeNodeHeader_ == NULL) {
            MemBlock* newBlock = new MemBlock;
            newBlock->pNext_ = NULL;

            freeNodeHeader_ = &newBlock->data[0];

            for (size_t i = 1; i < NumofObjects; ++i) {
                newBlock->data[i-1].pNext_ = &newBlock->data[i];
            }
            newBlock->data[NumofObjects - 1].pNext_ = NULL;

            if (memBlockHeader_ == NULL) {
                memBlockHeader_ = newBlock;
            } else {
                newBlock->pNext_ = memBlockHeader_;
                memBlockHeader_ = newBlock;
            }
       }

       assert(freeNodeHeader_ != NULL);
       void* freeNode = freeNodeHeader_;
       freeNodeHeader_ = freeNodeHeader_->pNext_;
       return freeNode;
    }
}

template<size_t ObjectSize, size_t NumofObjects>
void MemoryPool<ObjectSize, NumofObjects>::free(void* p)
{
    std::lock_guard<std::mutex> lock(mutex_);
    FreeNode* pNode = reinterpret_cast<FreeNode*>(p);
    pNode->pNext_ = freeNodeHeader_;
    freeNodeHeader_ = pNode;
}

}
#endif
