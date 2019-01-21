#pragma once
// #include <WebServer/Util.h>
// #include <WebServer/Slice.h>
#include "./Util.h"

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
    static const size_t KInitialSize;
    static const size_t ReservedPrependSize;

    Buffer(int iniatiasize = KInitialSize, int reserveSize = ReservedPrependSize)
        : readerIndex_(reserveSize),
          writerIndex_(reserveSize),
          capacity_(reserveSize + iniatiasize),
          reserved_prepend_size_(reserveSize),
          has_data_(false)
    {
        buffer_ = new char[capacity_];
        assert(length() == 0);
        assert(writeableBytes() == KInitialSize);
        assert(readableBytes() == 0);
        assert(prependableBytes() == reserved_prepend_size_);
        assert(!has_data_);
    }

    ~Buffer()
    {
        delete[] buffer_;
        buffer_ = nullptr;
    }

public:
    size_t length() const
    {
        return readableBytes();
    }

    char* data()
    {
        return buffer_ + readerIndex_;
    }

    const char* data() const
    {
        return buffer_ + readerIndex_;
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
        if (writerIndex_ > readerIndex_) {
            size_t pre = readerIndex_ > reserved_prepend_size_ ? readerIndex_ - reserved_prepend_size_ : 0;
            return (capacity_ - writerIndex_) + pre;
        }
        else if (writerIndex_ == readerIndex_) {
            if (has_data_) return 0;
            return capacity_ - reserved_prepend_size_;
        }
        else {
            return readerIndex_ - writerIndex_;
        }
    }

    size_t readableBytes() const
    {
        if (writerIndex_ > readerIndex_) {
            return writerIndex_ - readerIndex_;
        }
        else if (!has_data_) {
            return 0;
        }
        else {
            return (capacity_ - readerIndex_) + (writerIndex_ - reserved_prepend_size_);
        }
    }

    size_t prependableBytes()
    {
        assert(readerIndex_ >= 0);
        if (writerIndex_ > readerIndex_) {
            return readerIndex_;
        }
        if (!has_data_) {
            return reserved_prepend_size_;
        }
        return readerIndex_ - writerIndex_;
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
        std::swap(has_data_, rhs.has_data_);
    }

    void retrieve(size_t len)
    {
        assert(len >= 0);
        assert(len <= length());
        if (len == length()) {
            has_data_ = false;
            writerIndex_ = reserved_prepend_size_;
            readerIndex_ = reserved_prepend_size_;
            return;
        }
        if (!len || !length()) {
            return;
        }
        if (writerIndex_ > readerIndex_) {
            readerIndex_ += len;
            if (readerIndex_ == capacity_) {
                readerIndex_ = reserved_prepend_size_;
            }
        }
        else if (writerIndex_ == readerIndex_) {
            if (has_data_) {
                readerMove(len);
            }
            else {
                reset();
            }
        }
        else {
            readerMove(len);
        }
    }

    void readerMove(size_t len) {
        assert(len <= length());
        if (!len || !length()) return;
        if (writerIndex_ > readerIndex_) {
            readerIndex_ += len;
            return;
        }
        size_t surplus = capacity_ - readerIndex_;
        if (len < surplus) {
            readerIndex_ += len;
        }
        else {
            readerIndex_ = reserved_prepend_size_ + len - surplus;
        }
        if (readerIndex_ == capacity_) {
            readerIndex_ = reserved_prepend_size_;
        }
    }

    void reset()
    {
        writerIndex_ = reserved_prepend_size_;
        readerIndex_ = reserved_prepend_size_;
        has_data_ = false;
    }

    void ensureWriteableBytes(size_t len)
    {
        if (writeableBytes() < len)
        {
            growth(len);
        }
        assert(writeableBytes() >= len);
    }

public:
    void append(const char* data, size_t len)
    {
        ensureWriteableBytes(len);
        if (readerIndex_ > writerIndex_) {
            memcpy(writeBegin(), data, len);
            assert(writerIndex_ + len <= readerIndex_);
            writerIndex_ += len;
        }
        else {
            if (!has_data_) {
                memcpy(writeBegin(), data, len);
                writerIndex_ += len;
            }
            else {
                size_t surplus = capacity_ - writerIndex_;
                size_t prelen = len - surplus;
                if (surplus >= len) {
                    memcpy(writeBegin(), data, len);
                    writerIndex_ += len;
                }
                else {
                    memcpy(writeBegin(), data, surplus);
                    memcpy(buffer_ + reserved_prepend_size_, data + surplus, prelen);
                    writerIndex_ = reserved_prepend_size_ + prelen;
                }
            }
        }
        if (writerIndex_ == capacity_) {
            writerIndex_ = reserved_prepend_size_;
        }
        if (len > 0) has_data_ = true;
    }

    void append(const void* data, size_t len)
    {
        append(static_cast<const char*>(data), len);
    }

    void append(const std::string& str)
    {
        append(str.c_str(), str.size());
    }

    void appendInt8(int8_t x)
    {
        append(&x, sizeof x);
    }

    void appendInt16(uint16_t x) 
    {
        uint16_t s = sockets::hostToNetwork16(x);
        append(&s, sizeof s);
    }

    void appendInt32(int32_t x) {
        uint32_t s = sockets::hostToNetwork32(x);
        append(&s, sizeof s);
    }

    void appendInt64(uint64_t x) {
        uint64_t s = sockets::hostToNetwork64(x);
        append(&s, sizeof s);
    }

    void prepend(const void* data, size_t len)
    {
        assert(len <= reserved_prepend_size_);
        readerIndex_ -= len;
        const char* p = static_cast<const char*>(data);
        memcpy(buffer_ + readerIndex_, p, len);
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
    ////note that, peek just look the head of buffer
    ////just keep readerIndex_ in original position.
    
    void peekUtility(void* buffer, size_t len) const
    {
        char* buf = static_cast<char*>(buffer);
        if (writerIndex_ > readerIndex_) {
            ::memcpy(buf, data(), len);
        }
        else {
            size_t surplus = capacity_ - readerIndex_;
            if (len <= surplus) {
                ::memcpy(buf, data(), len);
            }
            else {
                size_t prelen = len - surplus;
                ::memcpy(buf, data(), surplus);
                ::memcpy(buf + surplus, begin() + reserved_prepend_size_, prelen);
            }
        }
    }

    int32_t peekInt32() const
    {
        assert(length() >= sizeof(int32_t));
        int32_t be32 = 0;
        char buffer[sizeof be32];
        peekUtility(buffer, sizeof buffer);
        ::memcpy(&be32, buffer, sizeof buffer);
        return be32toh(be32);
    }

    int16_t peekInt16() const
    {
        assert(length() >= sizeof(int16_t));
        int16_t be16 = 0;
        char buffer[sizeof be16];
        peekUtility(buffer, sizeof buffer);
        ::memcpy(&be16, buffer, sizeof buffer);
        return be16toh(be16);
    }

    int8_t peekInt8() const
    {
        assert(length() >= sizeof(int8_t));
        int8_t be8 = 0;
        char buffer[sizeof be8];
        peekUtility(buffer, sizeof buffer);
        ::memcpy(&be8, buffer, sizeof buffer);
        return be8;
    }

    const char* peek() const {
        return buffer_ + readerIndex_;
    }

    const char* findCRLF() const {
        if (!has_data_) return nullptr;
        if (writerIndex_ > readerIndex_) {
            const char* pos = std::search(data(), writeBegin(), KCRLF, KCRLF + 2);
            if (pos == writeBegin()) return nullptr;
            return pos;
        }
        const char* p1 = std::search(data(), begin() + capacity_, KCRLF, KCRLF + 2);
        if (p1 != begin() + capacity_) return p1;
        const char* p2 = std::search(begin() + reserved_prepend_size_, writeBegin(), KCRLF, KCRLF + 2);
        if (p2 != writeBegin()) return p2;
        if (buffer_[capacity_-1] == '\r' && buffer_[reserved_prepend_size_] == '\n') return begin() + capacity_ - 1;
        return nullptr;
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

    ////read data from sockfd
    ssize_t readFd(int sockfd, int *savedErrno);

    void nextString(size_t len, std::string& empty_str) {
        assert(len <= length());
        empty_str.reserve(len);
        if (writerIndex_ > readerIndex_) {
            empty_str.append(data(), len);
            retrieve(len);
            return;
        }
        if (!has_data_) {
            return;
        }
        size_t surplus = capacity_ - readerIndex_;
        size_t prelen = len - surplus;
        if (surplus >= len) {
            empty_str.append(data(), surplus);
            retrieve(len);
            return;
        }
        empty_str.append(begin() + reserved_prepend_size_, prelen);
        retrieve(prelen);
    }

    void nextAllString(std::string& empty_str) {
        return nextString(length(), empty_str);
    }

//public:
    //const char* findCRLF() const {
    //    const char* crlf = std::search(data(), writeBegin(), KCRLF, KCRLF + 2);
    //    return crlf == writeBegin() ? nullptr : crlf;
    //}

    //const char* findCRLF(const char* start) const {
    //    assert(data() <= start);
    //    assert(start <= writeBegin());
    //    const char* crlf = std::search(start, writeBegin(), KCRLF, KCRLF + 2);
    //    return crlf == writeBegin() ? nullptr : crlf;
    //}

    //const char* findEOL() const {
    //    const void* eol = ::memchr(data(), '\n', length());
    //    return static_cast<const char*>(eol);
    //}

    //const char* findEOL(const char* start) const {
    //    assert(pos >= data());
    //    assert(pos <= writeBegin());
    //    const void* eol = ::memchr(start, '\n', writeBegin() - start);
    //    return static_cast<const char*>(eol);
    //}
    
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
        assert(writeableBytes() < len);
        size_t newsize = (capacity_ << 1) + len;
        char* newbuf = new char[newsize];
        size_t readable = length();

        if (writerIndex_ > readerIndex_) {
            memcpy(newbuf, buffer_, reserved_prepend_size_);
            memcpy(newbuf + reserved_prepend_size_, data(), readable);
            readerIndex_ = reserved_prepend_size_;
            writerIndex_ = reserved_prepend_size_+ readable;
            capacity_ = newsize;
            delete[] buffer_;
            buffer_ = newbuf;
        }
        else {
            if (!has_data_) {
                readerIndex_ = reserved_prepend_size_;
                writerIndex_ = reserved_prepend_size_;
                has_data_ = false;
                capacity_ = newsize;
                delete[] buffer_;
                buffer_= newbuf;
            }
            else {  //has data, and the writerIndex_ <= readerIndex_ 
                size_t surplus = capacity_ - readerIndex_;
                size_t prelen = readable - surplus;
                memcpy(newbuf, buffer_, reserved_prepend_size_);
                memcpy(newbuf, data(), surplus);
                memcpy(newbuf, buffer_ + reserved_prepend_size_, prelen);
                readerIndex_ = reserved_prepend_size_;
                writerIndex_ = reserved_prepend_size_ + readable;
                has_data_ = true;
                capacity_ = newsize;
                delete[] buffer_;
                buffer_ = newbuf;
            }
        } //end if else
    }

private:
    char* buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
    size_t capacity_;
    size_t reserved_prepend_size_;
    bool has_data_;
    static const char KCRLF[];
};


}
}
