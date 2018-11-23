#include <boost/function.hpp>
#include <iostream>
#include <boost/bind.hpp>

class test {
public:
    using BindFunc = boost::function< void (int)>;
    test() {}
    ~test() = default;
    BindFunc bindfunc = boost::bind(&test::fun, this, _1);
public:
    static void stafun() {
        std::cout << "stafun() run >>> " << std::endl;
    }
    void fun(int i) {
        std::cout << "test::fun() run>>> " << i << std::endl;
    }
    void fun2() {
        std::cout << "fun2 is running...." << std::endl;
    }
};


int main() {
    test t;
    boost::function< void (int)>  fun = boost::bind(&test::fun, t, _1);
    fun(2);


    test* t2;
    auto fun2 = boost::bind(&test::fun, t2, _1);
    delete t2;
    fun2(100);
    /*******************/
    t.bindfunc(3);
    auto f = boost::bind(test::stafun);
    f(2);
    auto f2 = boost::bind(&test::fun2, _1);
}
