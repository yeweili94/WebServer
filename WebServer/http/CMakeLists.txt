#net静态库
set(http_SRCS
    HttpParser.cpp
    HttpResponse.cpp
    )
add_library(http ${http_SRCS})
target_link_libraries(http libserver_net)

install(TARGETS http DESTINATION lib)
#binary file

set(HttpParser.h
    HttpServer.h
    HttpRequest.h
    HttpResponse.h
    )
install(FILES ${HEADERS} DESTINATION include/WebServer/http)


add_executable(http_server HttpServer.cpp)
target_link_libraries(http_server http)
