add_executable(reactor_test01 Reactor_test01.cpp)
target_link_libraries(reactor_test01 libserver_net)

add_executable(reactor_test02 Reactor_test02.cpp)
target_link_libraries(reactor_test02 libserver_net)

add_executable(reactor_test03 Reactor_test03.cpp)
target_link_libraries(reactor_test03 libserver_net)

add_executable(reactor_test04 Reactor_test04.cpp)
target_link_libraries(reactor_test04 libserver_net)

add_executable(reactor_test05 Reactor_test05.cpp)
target_link_libraries(reactor_test05 libserver_net)

add_executable(EventLoopThread_test EventLoopThread_test.cpp)
target_link_libraries(EventLoopThread_test libserver_net)

add_executable(Acceptor_test Acceptor_test.cpp)
target_link_libraries(Acceptor_test libserver_net)

# add_executable(client client.cpp)
# target_link_libraries(client libserver_net)

# if (GTEST_FOUND)
    # add_executable(circular_buffer_test CirCularBufferUnitTest.cpp)
    # target_link_libraries(circular_buffer_test libserver_net gtest gtest_main)
# endif()
