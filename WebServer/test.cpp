#include "./MemoryPool.h"

using namespace ywl;

int main()
{
    test* t1 = new test;
    t1->print();

    test* t2 = new test;
    t2->print();

    test* t3 = new test;
    t3->print();

    test* t4= new test;
    t4->print();

    delete t1;
    delete t2;
    delete t3;
    delete t4;
}
