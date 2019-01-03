#net静态库
set(net_SRCS
    Channel.cpp
    EventLoop.cpp
    Timer.cpp
    EPollPoller.cpp
    EventLoopThread.cpp
    Util.cpp
    )
add_library(libserver_net ${net_SRCS})
target_link_libraries(libserver_net libserver_base)

install(TARGETS libserver_net DESTINATION lib)
#binary file

set(HEADERS
    Channel.h
    EventLoop.h
    EPollPoller.h
    Poller.h
    Timer.h
    TimerId.h
    )
install(FILES ${HEADERS} DESTINATION include/WebServer/net)

add_subdirectory(test)
