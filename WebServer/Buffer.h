// #include "./Slice.h"
// #include "./Util.h"
#include <WebServer/Slice.h>
#include <WebServer/Util.h>
#include <algorithm>

namespace ywl
{
namespace net
{

class Buffer
{
public:
    static const size_t kCheapPrependSize;
    static const size_t kInitialSize;

    explicit Buffer(size_t initial_size = kInitialSize, size_t reserved_prepend_size = kCheapPrependSize)
        : capacity_(reserved_prepend_size + initial_size),
          read_index_(reserved_prepend_size),
          write_index_(reserved_prepend_size),
          reserved_prepend_size_(reserved_prepend_size) {
        buffer_ = new char[capacity_];
        assert(length() == 0);
        assert(writableBytes() == initial_size);
        assert(PrependableBytes() == reserved_prepend_size);
    }

    ~Buffer() {
        delete[] buffer_;
        buffer_ = nullptr;
        capacity_ = 0;
    }

    void swap(Buffer& rhs) {
        std::swap(buffer_, rhs.buffer_);
        std::swap(capacity_, rhs.capacity_);
        std::swap(read_index_, rhs.read_index_);
        std::swap(write_index_, rhs.write_index_);
        std::swap(reserved_prepend_size_, rhs.reserved_prepend_size_);
    }

    // Skip advances the reading index of the buffer
    void skip(size_t len) {
        if (len < length()) {
            read_index_ += len;
        } else {
            reset();
        }
    }

    // Retrieve advances the reading index of the buffer
    // Retrieve it the same as Skip.
    void retrieve(size_t len) {
        skip(len);
    }

    // Truncate discards all but the first n unread bytes from the buffer
    // but continues to use the same allocated storage.
    // It does nothing if n is greater than the length of the buffer.
    void truncate(size_t n) {
        if (n == 0) {
            read_index_ = reserved_prepend_size_;
            write_index_ = reserved_prepend_size_;
        } else if (write_index_ > read_index_ + n) {
            write_index_ = read_index_ + n;
        }
    }

    void reset() {
        truncate(0);
    }

    void reserve(size_t len) {
        if (capacity_ >= len + reserved_prepend_size_) {
            return;
        }
        grow(len + reserved_prepend_size_);
    }

    // Make sure there is enough memory space to append more data with length len
    void ensureWritableBytes(size_t len) {
        if (writeableBytes() < len) {
            grow(len);
        }

        assert(writableBytes() >= len);
    }

    // ToText appends char '\0' to buffer to convert the underlying data to a c-style string text.
    // It will not change the length of buffer.
    void toText() {
        appendInt8('\0');
        unwriteBytes(1);
    }

#define swap_64(x)                          \
    ((((x) & 0xff00000000000000ull) >> 56)       \
     | (((x) & 0x00ff000000000000ull) >> 40)     \
     | (((x) & 0x0000ff0000000000ull) >> 24)     \
     | (((x) & 0x000000ff00000000ull) >> 8)      \
     | (((x) & 0x00000000ff000000ull) << 8)      \
     | (((x) & 0x0000000000ff0000ull) << 24)     \
     | (((x) & 0x000000000000ff00ull) << 40)     \
     | (((x) & 0x00000000000000ffull) << 56))

    // Write
public:
    void Write(const void* /*restrict*/ d, size_t len) {
        ensureWritableBytes(len);
        memcpy(writeBegin(), d, len);
        assert(write_index_ + len <= capacity_);
        write_index_ += len;
    }

    void append(const Slice& str) {
        Write(str.data(), str.size());
    }

    void append(const char* /*restrict*/ d, size_t len) {
        Write(d, len);
    }

    void append(const void* /*restrict*/ d, size_t len) {
        Write(d, len);
    }

    // Append int64_t/int32_t/int16_t with network endian
    void appendInt64(int64_t x) {
        int64_t be = swap_64(x);
        Write(&be, sizeof be);
    }

    void appendInt32(int32_t x) {
        int32_t be32 = htonl(x);
        Write(&be32, sizeof be32);
    }

    void appendInt16(int16_t x) {
        int16_t be16 = htons(x);
        Write(&be16, sizeof be16);
    }

    void appendInt8(int8_t x) {
        Write(&x, sizeof x);
    }

    // Prepend int64_t/int32_t/int16_t with network endian
    void prependInt64(int64_t x) {
        int64_t be = swap_64(x);
        prepend(&be, sizeof be);
    }

    void prependInt32(int32_t x) {
        int32_t be32 = htonl(x);
        prepend(&be32, sizeof be32);
    }

    void prependInt16(int16_t x) {
        int16_t be16 = htons(x);
        prepend(&be16, sizeof be16);
    }

    void prependInt8(int8_t x) {
        prepend(&x, sizeof x);
    }

    // Insert content, specified by the parameter, into the front of reading index
    void prepend(const void* /*restrict*/ d, size_t len) {
        assert(len <= PrependableBytes());
        read_index_ -= len;
        const char* p = static_cast<const char*>(d);
        memcpy(begin() + read_index_, p, len);
    }

    void unwriteBytes(size_t n) {
        assert(n <= length());
        write_index_ -= n;
    }

    void writeBytes(size_t n) {
        assert(n <= WritableBytes());
        write_index_ += n;
    }

    //Read
public:
    // Peek int64_t/int32_t/int16_t/int8_t with network endian
    int64_t readInt64() {
        int64_t result = peekInt64();
        skip(sizeof result);
        return result;
    }

    int32_t readInt32() {
        int32_t result = peekInt32();
        skip(sizeof result);
        return result;
    }

    int16_t readInt16() {
        int16_t result = peekInt16();
        skip(sizeof result);
        return result;
    }

    int8_t readInt8() {
        int8_t result = peekInt8();
        skip(sizeof result);
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

    // ReadFromFD reads data from a fd directly into buffer,
    ssize_t readFd(int fd, int* saved_errno);

    // Next returns a slice containing the next n bytes from the buffer,
    // advancing the buffer as if the bytes had been returned by Read.
    // If there are fewer than n bytes in the buffer, Next returns the entire buffer.
    // The slice is only valid until the next call to a read or write method.
    Slice next(size_t len) {
        if (len < length()) {
            Slice result(data(), len);
            read_index_ += len;
            return result;
        }

        return nextAll();
    }

    // NextAll returns a slice containing all the unread portion of the buffer,
    // advancing the buffer as if the bytes had been returned by Read.
    Slice nextAll() {
        Slice result(data(), length());
        reset();
        return result;
    }

    std::string nextString(size_t len) {
        Slice s = next(len);
        return std::string(s.data(), s.size());
    }

    std::string nextAllString() {
        Slice s = nextAll();
        return std::string(s.data(), s.size());
    }

    // ReadByte reads and returns the next byte from the buffer.
    // If no byte is available, it returns '\0'.
    char readByte() {
        assert(length() >= 1);

        if (length() == 0) {
            return '\0';
        }

        return buffer_[read_index_++];
    }

    // UnreadBytes unreads the last n bytes returned
    // by the most recent read operation.
    void unreadBytes(size_t n) {
        assert(n < read_index_);
        read_index_ -= n;
    }

    // Peek
public:
    // Peek int64_t/int32_t/int16_t/int8_t with network endian

    int64_t peekInt64() const {
        assert(length() >= sizeof(int64_t));
        int64_t be64 = 0;
        ::memcpy(&be64, data(), sizeof be64);
        return swap_64(be64);
    }

    int32_t peekInt32() const {
        assert(length() >= sizeof(int32_t));
        int32_t be32 = 0;
        ::memcpy(&be32, data(), sizeof be32);
        return ntohl(be32);
    }

    int16_t peekInt16() const {
        assert(length() >= sizeof(int16_t));
        int16_t be16 = 0;
        ::memcpy(&be16, data(), sizeof be16);
        return ntohs(be16);
    }

    int8_t peekInt8() const {
        assert(length() >= sizeof(int8_t));
        int8_t x = *data();
        return x;
    }

public:
    const char* data() const {
        return buffer_ + read_index_;
    }

    char* writeBegin() {
        return begin() + write_index_;
    }

    const char* writeBegin() const {
        return begin() + write_index_;
    }

    // length returns the number of bytes of the unread portion of the buffer
    size_t length() const {
        assert(write_index_ >= read_index_);
        return write_index_ - read_index_;
    }

    // size returns the number of bytes of the unread portion of the buffer.
    // It is the same as length().
    size_t size() const {
        return length();
    }

    // capacity returns the capacity of the buffer's underlying byte slice, that is, the
    // total space allocated for the buffer's data.
    size_t capacity() const {
        return capacity_;
    }

    size_t writeableBytes() const {
        assert(capacity_ >= write_index_);
        return capacity_ - write_index_;
    }

    size_t readableBytes() const {
        assert(write_index_ >= read_index_);
        return write_index_ - read_index_;
    }

    size_t prependableBytes() const {
        return read_index_;
    }

    // Helpers
public:
    const char* findCRLF() const {
        const char* crlf = std::search(data(), writeBegin(), kCRLF, kCRLF + 2);
        return crlf == writeBegin() ? nullptr : crlf;
    }

    const char* findCRLF(const char* start) const {
        assert(data() <= start);
        assert(start <= WriteBegin());
        const char* crlf = std::search(start, writeBegin(), kCRLF, kCRLF + 2);
        return crlf == writeBegin() ? nullptr : crlf;
    }

    const char* findEOL() const {
        const void* eol = memchr(data(), '\n', length());
        return static_cast<const char*>(eol);
    }

    const char* findEOL(const char* start) const {
        assert(data() <= start);
        assert(start <= WriteBegin());
        const void* eol = memchr(start, '\n', writeBegin() - start);
        return static_cast<const char*>(eol);
    }
private:

    char* begin() {
        return buffer_;
    }

    const char* begin() const {
        return buffer_;
    }

    void grow(size_t len) {
        if (writeableBytes() + prependableBytes() < len + reserved_prepend_size_) {
            //grow the capacity
            size_t n = (capacity_ << 1) + len;
            size_t m = length();
            char* d = new char[n];
            memcpy(d + reserved_prepend_size_, begin() + read_index_, m);
            write_index_ = m + reserved_prepend_size_;
            read_index_ = reserved_prepend_size_;
            capacity_ = n;
            delete[] buffer_;
            buffer_ = d;
        } else {
            // move readable data to the front, make space inside buffer
            assert(reserved_prepend_size_ < read_index_);
            size_t readable = length();
            memmove(begin() + reserved_prepend_size_, begin() + read_index_, length());
            read_index_ = reserved_prepend_size_;
            write_index_ = read_index_ + readable;
            assert(readable == length());
            assert(WritableBytes() >= len);
        }
    }

private:
    char* buffer_;
    size_t capacity_;
    size_t read_index_;
    size_t write_index_;
    size_t reserved_prepend_size_;
    static const char kCRLF[];
};

}
}
