#pragma once
#include <WebServer/Util.h>
#include <vector>
#include <algorithm>
#include <assert.h>

class Buffer
{
public:
    static const size_t KInitialSize = 1024;
    Buffer()
        : readerIndex_(0),
          writerIndex_(0)
    {
        capacity_ = KInitialSize;
        buffer_ = new char[KInitialSize];
        assert(readerIndex_ == 0);
        assert(writerIndex_ == 0);
    }
public:
    size_t capacity()
    {
        return capacity_;
    }
    size_t length() const {
        return kjjj
    }
private:
    char* buffer_;
    int readerIndex_;
    int writerIndex_;

    int capacity_;
};
