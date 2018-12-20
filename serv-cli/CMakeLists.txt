#string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
#set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
add_executable(echo_serv sock_serv.cpp readline.hpp)
add_executable(echo_cli sock_cli.cpp readline.hpp)
