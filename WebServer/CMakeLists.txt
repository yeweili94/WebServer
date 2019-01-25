#net静态库
set(net_SRCS
    Channel.cpp
    EventLoop.cpp
    Timer.cpp
    EventLoopThread.cpp
    Util.cpp
    Buffer.cpp
    InetAddress.cpp
    Acceptor.cpp
    TcpConnection.cpp
    TcpServer.cpp
    )
add_library(libserver_net ${net_SRCS})
target_link_libraries(libserver_net libserver_base)

install(TARGETS libserver_net DESTINATION lib)
#binary file

set(HEADERS
    Channel.h
    EventLoop.h
    Poller.h
    Timer.h
    TimerId.h
    Util.h
    Buffer.h
    InetAddress.h
    Acceptor.h
    TcpConnection.h
    TcpServer.h
    )
install(FILES ${HEADERS} DESTINATION include/WebServer/net)

add_subdirectory(test)
add_subdirectory(http)
