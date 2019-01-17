#include <WebServer/Buffer.h>

#include <gtest/gtest.h>

using namespace ywl;
using namespace ywl::net;

namespace {

TEST(Buffer, AppendRead) {
    Buffer buf;
    EXPECT_EQ(buf.length(), 0);
    EXPECT_EQ(buf.writeableBytes(), Buffer::KInitialSize);
    EXPECT_EQ(buf.prependableBytes(), Buffer::ReservedPrependSize);

    const std::string str(200, 'x');
    buf.append(str);
    EXPECT_EQ(buf.length(), str.size());
    EXPECT_EQ(buf.writeableBytes(), Buffer::KInitialSize - str.size());
    EXPECT_EQ(buf.prependableBytes(), Buffer::ReservedPrependSize);

    const std::string str2 = buf.nextString(50);
    EXPECT_EQ(buf.length(), str.size() - str2.size());
    EXPECT_EQ(buf.writeableBytes(), Buffer::KInitialSize - str.size());
    EXPECT_EQ(buf.prependableBytes(), Buffer::ReservedPrependSize + str2.size());
    EXPECT_STREQ(str2.c_str(), std::string(50, 'x').c_str());
    
    buf.append(str);
    EXPECT_EQ(buf.length(), 2 * str.size() - str2.size());
    EXPECT_EQ(buf.writeableBytes(), Buffer::KInitialSize - 2 * str.size());
    EXPECT_EQ(buf.prependableBytes(), Buffer::ReservedPrependSize + str2.size());

    const std::string str3 = buf.nextAllString();
    EXPECT_EQ(str3.size(), 350);
    EXPECT_EQ(buf.length(), 0);
    EXPECT_EQ(buf.writeableBytes(), Buffer::KInitialSize);
    EXPECT_EQ(buf.prependableBytes(), Buffer::ReservedPrependSize);
    EXPECT_STREQ(str3.c_str(), std::string(350, 'x').c_str());
}

TEST(Buffer, Growth1) {
    Buffer buf;
    buf.append(std::string(400, 'y'));
    EXPECT_EQ(buf.length(), 400);
    EXPECT_EQ(buf.writeableBytes(), Buffer::KInitialSize - 400);

    buf.retrieve(50);
    EXPECT_EQ(buf.length(), 350);
    EXPECT_EQ(buf.writeableBytes(), Buffer::KInitialSize - 400);
    EXPECT_EQ(buf.prependableBytes(), 50 + Buffer::ReservedPrependSize);

    buf.append(std::string(1000, 'z'));
    EXPECT_EQ(buf.length(), 1350);
    EXPECT_EQ(buf.prependableBytes(), Buffer::ReservedPrependSize);

    buf.reset();
    EXPECT_EQ(buf.length(), 0);
    EXPECT_EQ(buf.prependableBytes(), Buffer::ReservedPrependSize);
    EXPECT_TRUE(buf.writeableBytes() >= Buffer::KInitialSize * 2);
    EXPECT_EQ(buf.writeableBytes(), 3056);
}

TEST(Buffer, Growth2) {
    size_t preprend_size = 16;
    Buffer buf(Buffer::KInitialSize, preprend_size);
    buf.append(std::string(400, 'y'));
    EXPECT_EQ(buf.length(), 400);
    EXPECT_EQ(buf.writeableBytes(), Buffer::KInitialSize - 400);
    EXPECT_EQ(buf.prependableBytes(), preprend_size);

    buf.retrieve(50);
    EXPECT_EQ(buf.length(), 350);
    EXPECT_EQ(buf.writeableBytes(), Buffer::KInitialSize - 400);
    EXPECT_EQ(buf.prependableBytes(), preprend_size + 50);

    buf.append(std::string(1000, 'z'));
    EXPECT_EQ(buf.length(), 1350);
    EXPECT_EQ(buf.writeableBytes(), (Buffer::KInitialSize)*2+1000-1350+16);
}

TEST(Buffer, InsideGrowth) {
    Buffer buf;
    buf.append(std::string(800, 'y'));
    EXPECT_EQ(buf.length(), 800);
    EXPECT_EQ(buf.writeableBytes(), Buffer::KInitialSize - 800);

    buf.retrieve(500);
    EXPECT_EQ(buf.length(), 300);
    EXPECT_EQ(buf.writeableBytes(), Buffer::KInitialSize - 800);
    EXPECT_EQ(buf.prependableBytes(), Buffer::ReservedPrependSize + 500);

    buf.append(std::string(300, 'z'));
    EXPECT_EQ(buf.length(), 600);
    EXPECT_EQ(buf.writeableBytes(), Buffer::KInitialSize - 600);
    EXPECT_EQ(buf.prependableBytes(), Buffer::ReservedPrependSize);
}

TEST(Buffer, Prepend) {
    Buffer buf;
    buf.append(std::string(200, 'x'));
    EXPECT_EQ(buf.length(), 200);
    EXPECT_EQ(buf.writeableBytes(), Buffer::KInitialSize - 200);
    EXPECT_EQ(buf.prependableBytes(), Buffer::ReservedPrependSize);


    int x = 0;
    buf.prepend(&x, sizeof x);
    EXPECT_EQ(buf.length(), 204);
    EXPECT_EQ(buf.writeableBytes(), Buffer::KInitialSize - 200);
    EXPECT_EQ(buf.prependableBytes(), Buffer::ReservedPrependSize - 4);
}

TEST(Buffer, FindEOL) {
    Buffer buf;
    buf.append(std::string(100000, 'x'));
    const char* null = nullptr;
    EXPECT_EQ(buf.findEOL(), null);
    EXPECT_EQ(buf.findEOL(buf.data() + 80000), null);
}

TEST(Buffer, FindEOL2) {
    Buffer buf;
    char b[32] = {0};
    b[4] = '\r';
    b[5] = '\n';
    buf.append(b, sizeof b);
    EXPECT_TRUE(buf.findEOL() != nullptr);
    EXPECT_TRUE(buf.findCRLF() != nullptr);
}

}
