#include <WebServer/base/Logging.h>
#include <WebServer/base/Thread.h>
#include <WebServer/base/Timestamp.h>

#include <boost/shared_ptr.hpp>

#include <string>
#include <unistd.h>
#include <vector>
#include <memory>
#include <iostream>

using namespace std;
using namespace ywl;

void threadFunc()
{
    for (int i = 0; i < 10000000; ++i)
    {
        LOG << i;
    }
}

void type_test()
{
    cout << "----------type test-----------" << endl;
    LOG << 0;
    LOG << 1234567890123;
    LOG << 1.0f;
    LOG << 3.1415926;
    LOG << (short) 1;
    LOG << (long long) 1;
    LOG << (unsigned int) 1;
    LOG << (unsigned long) 1;
    LOG << (long double) 1.6555556;
    LOG << (unsigned long long) 1;
    LOG << 'c';
    LOG << "abcdefg";
    LOG << string("This is a string");
}

void stressing_single_thread()
{
    // 100000 lines
    cout << "----------stressing test single thread-----------" << endl;
    for (int i = 0; i < 100000; ++i)
    {
        LOG << i;
    }
}

void stressing_multi_threads(int threadNum = 4)
{
    // threadNum * 100000 lines
    cout << "----------stressing test multi thread-----------" << endl;
    vector<boost::shared_ptr<Thread> > vsp;
    for (int i = 0; i < threadNum; ++i)
    {
        boost::shared_ptr<Thread> tmp(new Thread(threadFunc, "testFunc"));
        vsp.push_back(tmp);
    }
    for (int i = 0; i < threadNum; ++i)
    {
        vsp[i]->start();
    }
    for (int i = 0; i < threadNum; i++)
    {
        vsp[i]->join();
    }
}

void other()
{
    // 1 line
    cout << "----------other test-----------" << endl;
    LOG << "fddsa" << 'c' << 0 << 3.666 << string("This is a string");
}

int main()
{
    // 共500014行
    Timestamp now(Timestamp::now());
    std::cout << now.toFormattedString() << std::endl;
    // type_test();
    // stressing_single_thread();
    // other();
    stressing_multi_threads();
    Timestamp now1(Timestamp::now());
    std::cout << now1.toFormattedString() << std::endl;
    return 0;
}
