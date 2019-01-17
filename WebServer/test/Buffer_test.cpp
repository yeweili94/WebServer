#include <WebServer/Buffer.h>
#include <unistd.h>
#include <string>
#include <iostream>
using namespace std;

using namespace ywl;
using namespace ywl::net;
using std::string;

void testBufferRead(Buffer& buf)
{
    assert(buf.length() == 0);
    assert(buf.writeableBytes() == 0);
    assert(buf.prependableBytes() == 0);
    buf.retrieveAllAsString();
}

void testAppend(Buffer& buf, size_t x)
{
    std::string str(x, 'x');
    buf.append(str.c_str(), str.size());
    assert(buf.size() == str.size());
    assert(buf.writeableBytes() == Buffer::KInitialSize - str.size());
    assert(buf.prependableBytes() == Buffer::ReservedPrependSize);
}

int main()
{
    Buffer buffer;
    testBufferRead(buffer);
    testAppend(buffer, 200);
    std::cout << buffer.nextAllString() << std::endl;
    sleep(1);
    printf("done!!!!\n");
    testAppend(buffer, 1026);
    std::cout << buffer.nextAll().toString() << std::endl;
    sleep(1);
    printf("done!!!!\n");
    testAppend(buffer, 200000);
}
