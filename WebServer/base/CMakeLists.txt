# 并将名称保存到 DIR_SRCS 变量
set(base_SRC
    Condition.cpp
    Thread.cpp
    CountDownLatch.cpp
    Timestamp.cpp
    ThreadPool.cpp
    )
add_library(libserver_base ${base_SRC})
target_link_libraries(libserver_base pthread rt)
set_target_properties(libserver_base PROPERTIES OUTPUT_NAME "server_base")
add_subdirectory(tests)
