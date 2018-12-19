cmake_minimum_required (VERSION 2.8)
project (modern_cpp)
# 添加子目录
add_subdirectory(./base)
# 指定生成目标 
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
add_executable(weakcallback weakcallback.cpp)
# 添加链接库
target_link_libraries(weakcallback thread)
