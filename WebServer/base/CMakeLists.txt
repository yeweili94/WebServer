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

install(TARGETS libserver_base DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/WebServer/base)

add_subdirectory(tests)
