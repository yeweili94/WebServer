#ifndef WEBSERVER_SLICE_H
#define WEBSERVER_SLICE_H

#include <string.h>
#include <assert.h>
#include <string>

//
// Slice is a simple structure containing a pointer into some external
// storage and a size.  The user of a Slice must ensure that the slice
// is not used after the corresponding external storage has been
// deallocated.
//
namespace ywl
{

// copy from leveldb project
// @see https://github.com/google/leveldb/blob/master/include/leveldb/slice.h
// Slice类不拥有资源，再包裹的资源析构之前使用
class Slice
{
public:
    typedef char value_type;

    Slice() : data_(""),
              size_(0)
    {
    }
    Slice(const char* d, size_t n)
        : data_(d),
          size_(n)
    {
    }
    Slice(const std::string& s)
        : data_(s.data()),
          size_(s.size())
    {
    }
    Slice(const char* d)
        : data_(d),
          size_(strlen(d))
    {
    }
    const char* data() const {
        return data_;
    }

    size_t size() const
    {
        return size_;
    }
    bool empty() const {
        return size_ == 0;
    }
    char operator[](size_t n) const {
        assert(n < size_);
        return data_[n];
    }
    void clear()
    {
        data_ = "";
        size_ = 0;
    }
    void remove_prefix(size_t n) {
        assert(n <= size());
        data_ += n;
        size_ -= n;
    }
    std::string toString() const {
        return std::string(data_, size_);
    }
    int compare(const Slice& b) const;
private:
    const char* data_;
    size_t size_;
};

//----------------------------------------------------------
//typedef Map<Slice, Slice> SliceSliceMap
//----------------------------------------------------------

inline bool operator==(const Slice& lhs, const Slice& rhs) {
    return ((lhs.size() == rhs.size()) &&
            (memcmp(lhs.data(), rhs.data(), lhs.size()) == 0));
}

inline bool operator!=(const Slice& lhs, const Slice& rhs) {
    return !(lhs == rhs);
}

inline bool operator<(const Slice& lhs, const Slice& rhs) {
    return lhs.compare(rhs) < 0;
}

inline int Slice::compare(const Slice& b) const {
    const size_t min_len = (size_ < b.size_) ? size_ : b.size_;
    int ret = memcmp(data_, b.data_, min_len);
    if (ret == 0) {
        if (size_ > b.size_) {
            return 1;
        }
        else if (size_ == b.size_) {
            return 0;
        } else {
            return -1;
        }
    }
    return ret;
}

}

#endif
