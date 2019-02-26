#include <WebServer/base/LogStream.h>

#include <boost/static_assert.hpp>

#include <limits>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <algorithm>

const char digits[] = "9876543210123456789";
const char* zero = digits + 9;
BOOST_STATIC_ASSERT(sizeof(digits) == 20);

const char digitsHex[] = "0123456789ABCDEF";
BOOST_STATIC_ASSERT(sizeof(digitsHex) == 17);

template<typename T>
size_t convert(char buf[], T value)
{
    T i = value;
    char* p = buf;

    do
    {
        int lsd = static_cast<int> (i % 10);
        i /= 10;
        *p++ = zero[lsd];
    } while (i != 0);

    if (value < 0)
    {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}

size_t convertHex(char buf[], uintptr_t value)
{
    uintptr_t i = value;
    char* p = buf;

    do
    {
        int lsd = i % 16;
        i /= 16;
        *p++ = digitsHex[lsd];
    } while(i != 0);

    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}

using namespace ywl;
template class ywl::FixedBuffer<KSmallBuffer>;
template class ywl::FixedBuffer<KLargeBuffer>;

template<typename T>
void LogStream::formatInteger(T v)
{
    //buffer容不下KMaxNumericSize则丢弃
    if (buffer_.writeableBytes() >= KMaxNumericSize)
    {
        size_t len = convert(buffer_.current(), v);
        buffer_.retrieve(len);
    }
}

LogStream& LogStream::operator<<(short v)

{
    *this << static_cast<int>(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned short v)
{
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream& LogStream::operator<<(int v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(const void* p)
{
    uintptr_t v = reinterpret_cast<uintptr_t>(p);
    if (buffer_.writeableBytes() >= KMaxNumericSize)
    {
        char* buf = buffer_.current();
        buf[0] = '0';
        buf[1] = 'x';
        size_t len = convertHex(buf+2, v);
        buffer_.retrieve(len+2);
    }
    return *this;
}

LogStream& LogStream::operator<<(double v)
{
    if (buffer_.writeableBytes() >= KMaxNumericSize)
    {
        int len = snprintf(buffer_.current(), KMaxNumericSize, "%.12g", v);
        buffer_.retrieve(len);
    }
    return *this;
}

LogStream& LogStream::operator<<(long double v)
{
    if (buffer_.writeableBytes() >= KMaxNumericSize)
    {
        int len = snprintf(buffer_.current(), KMaxNumericSize, "%.12Lg", v);
        buffer_.retrieve(len);
    }
    return *this;
}

