#ifndef WEBSERVER_MEMORY_POOL_H
#define WEBSERVER_MEMORY_POOL_H

#include <boost/noncopyable.hpp>

#include <assert.h>
#include <stdio.h>

namespace ywl
{

template<size_t ObjectSize, size_t NumofObjects = 4096>
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
            ptr = memBlockHeader_->pNext;
            delete memBlockHeader_;
            memBlockHeader_ = ptr;
        }
    }

    void* malloc();
    void free(void*);

private:
    struct FreeNode
    {
        FreeNode* pNext;
        char data[ObjectSize];
    };

    struct MemBlock
    {
        MemBlock* pNext;
        FreeNode data[NumofObjects];
    };

    FreeNode* freeNodeHeader_;
    MemBlock* memBlockHeader_;
};

template<size_t ObjectSize, size_t NumofObjects>
void* MemoryPool<ObjectSize, NumofObjects>::malloc()
{
    if (freeNodeHeader_ == NULL) {
        MemBlock* newBlock = new MemBlock;
        newBlock->pNext = NULL;

        freeNodeHeader_ = &newBlock->data[0];

        for (size_t i = 1; i < NumofObjects; ++i) {
            newBlock->data[i-1].pNext = &newBlock->data[i];
        }
        newBlock->data[NumofObjects - 1].pNext = NULL;

        if (memBlockHeader_ == NULL) {
            memBlockHeader_ = newBlock;
        } else {
            newBlock->pNext = memBlockHeader_;
            memBlockHeader_ = newBlock;
        }
    }

    assert(freeNodeHeader_ != NULL);
    void* freeNode = freeNodeHeader_;
    freeNodeHeader_ = freeNodeHeader_->pNext;
    return freeNode;
}

template<size_t ObjectSize, size_t NumofObjects>
void MemoryPool<ObjectSize, NumofObjects>::free(void* p)
{
    FreeNode* pNode = reinterpret_cast<FreeNode*>(p);
    pNode->pNext = freeNodeHeader_;
    freeNodeHeader_ = pNode;
}

}
#endif
