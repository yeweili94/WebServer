#ifndef WEBSERVER_BUFFER_H
#define WEBSERVER_BUFFER_H

#include <WebServer/Util.h>

#include <boost/implicit_cast.hpp>

#include <vector>
#include <algorithm>
#include <string>

#include <assert.h>
#include <string.h>

namespace ywl
{
namespace net
{

class Buffer
{
public:
    static const size_t KCheapPrepend = 8;
    static const size_t KInitialSize = 1024;

    Buffer()
        :  buffer_(KCheapPrepend + KInitialSize),
           readerIndex_(KCheapPrepend),
           writerIndex_(KCheapPrepend)
    {
        assert(readableBytes() == 0);
        assert(writeableBytes() == 0);
        assert(prependableBytes() == KCheapPrepend);
    }
    void swap(Buffer& rhs)
    {
        buffer_.swap(rhs.buffer_);
        std::swap(rhs.readerIndex_, this->readerIndex_);
        std::swap(rhs.writerIndex_, this->writerIndex_);
    }
    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }
    size_t writeableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }
    size_t prependableBytes() const
    {
        return readerIndex_;
    }
    const char* peek() const
    {
        return begin() + readerIndex_;
    }
    char* peek()
    {
        return begin() + readerIndex_;
    }
    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }
    char* beginWrite()
    {
        return begin() + writerIndex_;
    }
    const char* findCRLF() const
    {
        const char* pos = std::search(peek(), beginWrite(), KCRLF, KCRLF + 2);
        return pos == beginWrite() ? NULL : pos;
    }
    const char* findCRLF(const char* start) const
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const char* pos = std::search(start, beginWrite(), KCRLF, KCRLF + 2);
        return pos == beginWrite() ? NULL : pos;
    }
    //取数据后重新移动下标index
    void retrieveAll()
    {
        readerIndex_ = KCheapPrepend;
        writerIndex_ = KCheapPrepend;
    }
    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        if (len < readableBytes()) {
            readerIndex_ += len;
        }
        else {
            retrieveAll();
        }
    }
    void retrieveUntil(const char* end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }
    std::string retrieveAsString(size_t len)
    {
        assert(len <= readableBytes());
        std::string ret(peek(), len);
        retrieve(len);
        return ret;
    }
    //向缓冲区中写入数据
    void ensureWriteableBytes(size_t len)
    {
        if (writeableBytes() < len)
        {
            enlargeSpace(len);
        }
        assert(writeableBytes() >= len);
    }
    void append(const char* data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        hasWritten(len);
    }
    void append(const void* data, size_t len)
    {
        append(static_cast<const char*>(data), len);
    }
        void hasWritten(size_t len)
    {
        writerIndex_ += len;
    }
        //prepend intxx in network endian
    void prepend(const void* data, size_t len)
    {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d+len, begin()+readerIndex_);
    }
    void shrink(size_t reserve)
    {
        Buffer other;
        other.ensureWriteableBytes(readableBytes()+reserve);
        other.append(begin()+readerIndex_, readableBytes()+reserve);
        swap(other);
    }
    ssize_t readFd(int fd, int* savedErrno);
private:
    void enlargeSpace(size_t len)
    {
        assert(writeableBytes() < len);
        if (writeableBytes() + prependableBytes() - KCheapPrepend < len)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            assert(KCheapPrepend <= readerIndex_);
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_,
                      begin() + KCheapPrepend);
            readerIndex_ = KCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
            assert(readable == readableBytes());
        }
    }
    const char* begin() const
    {
        return &*buffer_.begin();
    }
    char* begin()
    {
        return &*buffer_.begin();
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
    static const char KCRLF[];
};


}
}


#endif
