cmake_minimum_required (VERSION 2.8)
# 并将名称保存到 DIR_SRCS 变量
add_library (thread Thread.cpp)
TARGET_LINK_LIBRARIES(thread pthread)
