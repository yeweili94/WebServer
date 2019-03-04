# High Performance C++ WebServer

## Introduction
本项目为C++11编写的Web服务器，写这个项目的目的是为了学习，加深对网络编程的理解，以及
熟悉C++11智能指针的使用，项目解析了Get, Head等HTTP协议，支持长连接，并且实现了异步日志系统。

## Build
./build.sh

## Technical Point
* 基于reactor模式，epoll水平触发的http服务
* 循环缓冲异步日志
* 实现内存池，优化短连接性能
* valgrind检测内存泄漏

## TODO
* 线程池线程动态减少 (done)
* 任务队列可以采用无锁队列,避免加锁开销

## Usage
编译之前确认已经安装boost(必须), 和googletest(选择)
sudo apt-get install libboost-dev

enjoying...
