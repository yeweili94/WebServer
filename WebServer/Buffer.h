#pragma once
#include <WebServer/Util.h>
#include <WebServer/Slice.h>

#include <boost/noncopyable.hpp>

#include <algorithm>
#include <assert.h>
#include <string.h>
namespace ywl
{
namespace net
{

class Buffer : boost::noncopyable
{
public:
    static const size_t KInitialSize = 1024;
    static const size_t ReservedPrependSize = 8;

    Buffer(int iniatiasize = KInitialSize, int reserveSize = ReservedPrependSize)
        : readerIndex_(reserveSize),
          writerIndex_(reserveSize),
          capacity_(reserveSize + iniatiasize),
          reserved_prepend_size_(reserveSize)
    {
        buffer_ = new char[capacity_];
        assert(length() == 0);
        assert(writeableBytes() == KInitialSize);
        assert(readableBytes() == 0);
        assert(prependableBytes() == reserved_prepend_size_);
    }

    ~Buffer()
    {
        delete[] buffer_;
        buffer_ = nullptr;
    }
public:
    size_t length() const
    {
        assert(writerIndex_ >= readerIndex_);
        return readableBytes();
    }
    size_t size() const
    {
        return length();
    }
    char* data()
    {
        return buffer_ + readerIndex_;
    }
    const char* data() const
    {
        return buffer_+readerIndex_;
    }
    char* writeBegin()
    {
        return buffer_ + writerIndex_;
    }
    const char* writeBegin() const
    {
        return buffer_ + writerIndex_;
    }
    size_t writeableBytes()
    {
        assert(writerIndex_ <= capacity_);
        return capacity_ - writerIndex_;
    }
    size_t readableBytes() const
    {
        assert(writerIndex_ >= readerIndex_);
        return writerIndex_ - readerIndex_;
    }
    size_t prependableBytes()
    {
        assert(readerIndex_ >= 0);
        return readerIndex_;
    }
    size_t capacity()
    {
        return capacity_;
    }

public:
    void swap(Buffer& rhs) {
        std::swap(buffer_, rhs.buffer_);
        std::swap(capacity_, rhs.capacity_);
        std::swap(readerIndex_, rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
        std::swap(reserved_prepend_size_, rhs.reserved_prepend_size_);
    }

    void skip(size_t len)
    {
        if (len < length())
        {
            readerIndex_  += len;
        }
        else
        {
            reset();
        }
    }

    void retrieve(size_t len)
    {
        assert(len <= length());
        skip(len);
    }

    std::string retrieveAllAsString()
    {
        std::string ret(data(), readableBytes());
        retrieve(readableBytes());
        return ret;
    }

    //只保留前n个字符,n == 0 时重置位置
    void truncate(size_t n)
    {
        assert(writerIndex_ >= readerIndex_);
        if (n == 0) {
            readerIndex_ = reserved_prepend_size_;
            writerIndex_ = reserved_prepend_size_;
        } else {
            writerIndex_ = readerIndex_ + n;
        }
    }
    void reset()
    {
        truncate(0);
    }
    void reserve(size_t len)
    {
        if (capacity_ - reserved_prepend_size_ >= len) {
            return;
        }
        growth(len + reserved_prepend_size_);
    }
    void ensureWriteableBytes(size_t len)
    {
        if (writeableBytes() < len)
        {
            growth(len);
        }
        assert(writeableBytes() >= len);
    }

    void toCstring()
    {
        appendInt8('\0');
        writerIndex_ += 1;
    }

public:
    void append(const char* data, size_t len)
    {
        ensureWriteableBytes(len);
        memcpy(writeBegin(), data, len);
        assert(writerIndex_ + len <= capacity_);
        writerIndex_ += len;
    }
    void append(const void* data, size_t len)
    {
        append(static_cast<const char*>(data), len);
    }
    void append(std::string str)
    {
        append(str.c_str(), str.size());
    }
    void appendInt8(int8_t x)
    {
        append(&x, sizeof x);
    }
    void append(const Slice& slice) {
        append(slice.data(), slice.size());
    }
    void prepend(const void* data, size_t len)
    {
        assert(len <= readerIndex_);
        const char* p = static_cast<const char*>(data);
        memcpy(begin()+readerIndex_, p, len);
        readerIndex_ -= len;
    }
    void prependInt16(int16_t x)
    {
        int16_t be16 = be16toh(x);
        prepend(&be16, sizeof be16);
    }
    void prependInt32(int32_t x)
    {
        int32_t be32 = be32toh(x);
        prepend(&be32, sizeof be32);
    }

public: // peekIntxx
    int32_t peekInt32() const
    {
        assert(length() >= sizeof(int32_t));
        int32_t be32 = 0;
        ::memcpy(&be32, data(), sizeof be32);
        return be32toh(be32);
    }
    int16_t peekInt16() const
    {
        assert(length() >= sizeof(int16_t));
        int16_t be16 = 0;
        ::memcpy(&be16, data(), sizeof be16);
        return be16toh(be16);
    }
     int8_t peekInt8() const
    {
        assert(length() >= sizeof(int8_t));
        int16_t be8 = 0;
        ::memcpy(&be8, data(), sizeof be8);
        return be8;
    }

public: //readIntxx
    int32_t readInt32()
    {
        int32_t result = peekInt32();
        retrieve(sizeof result);
        return result;
    }
    int16_t readInt16()
    {
        int16_t result = peekInt16();
        retrieve(sizeof result);
        return result;
    }
    int8_t readInt8()
    {
        int8_t result = peekInt8();
        retrieve(sizeof result);
        return result;
    }
    Slice toSlice() const {
        return Slice(data(), length());
    }
    std::string toString() const {
        return std::string(data(), length());
    }
    void shrink(size_t reserve) {
        Buffer other(length() + reserve);
        other.append(toSlice());
        swap(other);
    }
    //read data from sockfd
    ssize_t readFd(int sockfd, int *savedErrno);

    Slice Next(size_t len) {
        if (len < length()) {
            Slice result(data(), len);
            readerIndex_ += len;
            return result;
        }
        return nextAll();
    }

    Slice nextAll() {
        Slice result(data(), length());
        reset();
        return result;
    }

    std::string nextString(size_t len) {
        Slice result = Next(len);
        return std::string(result.data(), result.size());
    }

    std::string nextAllString() {
        Slice s = nextAll();
        return std::string(s.data(), s.size());
    }

private:
    char* begin()
    {
        return buffer_;
    }
    const char* begin() const
    {
        return buffer_;
    }
    void growth(size_t len)
    {
        if (writeableBytes() + prependableBytes() < len + reserved_prepend_size_) {
            size_t newsize = (capacity_ << 1) + len;
            size_t oldLen = length();
            char* newbuf = new char[newsize];
            memcpy(newbuf + reserved_prepend_size_, data(), oldLen);
            writerIndex_ = reserved_prepend_size_ + oldLen;
            readerIndex_ = reserved_prepend_size_;
            capacity_ = newsize;
            delete[] buffer_;
            buffer_ = newbuf;
        } else {
            assert(reserved_prepend_size_ < readerIndex_);
            size_t readable = length();
            memmove(begin() + reserved_prepend_size_, begin() + readerIndex_, readable);
            readerIndex_ = reserved_prepend_size_;
            writerIndex_ = readerIndex_ + readable;
            assert(readable == length());
            assert(writeableBytes() >= len);
        }
    }

private:
    char* buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
    size_t capacity_;
    size_t reserved_prepend_size_;
    static const char KCRLF[];
};


}
}
