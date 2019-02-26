#ifndef WEB_SERVER_LOG_STREAM_H
#define WEB_SERVER_LOG_STREAM_H

#include <boost/noncopyable.hpp>

#include <string.h>
#include <assert.h>
#include <string>

class AsyncLogging;
const int KSmallBuffer = 4000;
const int KLargeBuffer = 4000 * 1024;

namespace ywl
{
template<size_t SIZE>
class FixedBuffer : boost::noncopyable
{
public:
    enum STATUS { FULL, FREE };

    FixedBuffer() 
        : next_(NULL),
          cur_(data_),
          status_(FREE)
    {
    }

    ~FixedBuffer()
    {
    }

    void append(const char* buf, size_t len)
    {
        if (writeableBytes() > static_cast<int>(len))
        {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    STATUS status() { return status_; }
    void setStatus(STATUS stat) { status_ = stat; }

    const char* data() const { return data_; }
    int length() const { return static_cast<int>(cur_ - data_); }

    char* current() { return cur_; }
    int writeableBytes() const { return static_cast<int>(end() - cur_); }
    void retrieve(size_t len) { cur_ += len; }

    void reset() {cur_ = data_;}
    void bzero() {memset(data_, 0, sizeof data_); }

    FixedBuffer<KLargeBuffer>* next_;

private:
    const char* end() const { return data_ + sizeof(data_); }

    char data_[SIZE];
    char* cur_;

    STATUS status_;
};

class LogStream : boost::noncopyable
{
public:
    typedef FixedBuffer<KSmallBuffer> Buffer;

    LogStream& operator<<(bool v)
    {
        buffer_.append(v ? "1" : "0", 1);
        return *this;
    }

    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(const void*);

    LogStream& operator<<(float v)
    {
        *this << static_cast<double>(v);
        return *this;
    }
    LogStream& operator<<(double);
    LogStream& operator<<(long double);

    LogStream& operator<<(char v)
    {
        buffer_.append(&v, 1);
        return *this;
    }
    LogStream& operator<<(const char* str)
    {
        if (str)
        {
            buffer_.append(str, strlen(str));
            return *this;
        }
        else
        {
            buffer_.append("(null)", 6);
        }
        return *this;
    }

    LogStream& operator<<(const unsigned char* str)
    {
        return operator<<(reinterpret_cast<const char*>(str));
    }

    LogStream& operator<<(const std::string& str)
    {
        buffer_.append(str.c_str(), str.size());
        return *this;
    }

    void append(const char* data, int len) { buffer_.append(data, len); }
    const Buffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }

private:
    template<typename T>
    void formatInteger(T);

    Buffer buffer_;

    static const int KMaxNumericSize = 32;
};

}//ywl

#endif
