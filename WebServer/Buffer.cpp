#include <WebServer/Buffer.h>

using namespace ywl;
using namespace ywl::net;

const size_t Buffer::KCheapPrepend;
const size_t Buffer::KInitialSize;
const char Buffer::KCRLF[] = "\r\n";

ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    char extrabuf[65536]; //64kb
    struct iovec vec[2];
    const size_t writable = writeableBytes();
    //第一块缓冲区
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writeableBytes();

    //第二块缓冲区
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = 65536;
    const ssize_t n = sockets::Readv(fd, vec, 2);
    if (n < 0) {
        *savedErrno = errno;
    } else if (boost::implicit_cast<size_t>(n) <= writable) {
        writerIndex_ += n;
    } else {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}
