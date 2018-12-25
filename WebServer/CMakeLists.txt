#net静态库
add_subdirectory(base)
set(net_SRCS
    Channel.cpp
    EventLoop.cpp
    Timer.cpp
    EPollPoller.cpp
    )
add_library(libserver_net ${net_SRCS})
target_link_libraries(libserver_net libserver_base pthread rt)

#binary file
set(PROJECT_SRCS
    weakcallback.cpp
    )
add_executable(webserver ${PROJECT_SRCS})
add_subdirectory(test)
target_link_libraries(webserver libserver_net)
