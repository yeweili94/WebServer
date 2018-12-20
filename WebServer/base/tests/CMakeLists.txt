add_executable(reverse_test test.cpp)
add_executable(thread_test thread_test.cpp)
target_link_libraries(thread_test libserver_base)
