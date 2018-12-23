set(base_SRC
    Condition.cpp
    Thread.cpp
    CountDownLatch.cpp
    Timestamp.cpp
    ThreadPool.cpp
    FileUtil.cpp
    LogFile.cpp
    LogStream.cpp
    AsyncLogging.cpp
    Logging.cpp
    )
add_library(libserver_base ${base_SRC})
target_link_libraries(libserver_base pthread rt)
set_target_properties(libserver_base PROPERTIES OUTPUT_NAME "server_base")
add_subdirectory(tests)
