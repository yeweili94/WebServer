#net静态库
set(net_SRCS
    Channel.cpp
    EventLoop.cpp
    Timer.cpp
    )
add_library(libserver_net ${net_SRCS})
target_link_libraries(libserver_net pthread rt)

#binary file
set(PROJECT_SRCS
    weakcallback.cpp
    )
add_executable(webserver ${PROJECT_SRCS})
add_subdirectory(base)
target_link_libraries(webserver libserver_base libserver_net)
