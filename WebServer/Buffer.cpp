#include <WebServer/Buffer.h>

using namespace ywl;
using namespace ywl::net;

const char Buffer::KCRLF[] = "\r\n";
const size_t Buffer::KInitialSize = 1024;
const size_t Buffer::ReservedPrependSize = 8;

ssize_t Buffer::readFd(int sockfd, int *savedErrno)
{
    char extrabuf[1024*64];
    struct iovec vec[2];
    const size_t writable = writeableBytes();
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(sockfd, vec, iovcnt);

    if (n < 0)
    {
        *savedErrno = errno;
    }
    else if (static_cast<size_t>(n) <= writable)
    {
        writerIndex_ += n;
    }
    else
    {
        writerIndex_ = capacity_;
        append(extrabuf, n - writable);
    }

    return n;
}
