cmake_minimum_required(VERSION 3.0)

project(CTCPSERVER)

include_directories(${CMAKE_SOURCE_DIR}/include)
set(CMAKE_BUILD_TYPE Debug)

# 搜索源文件和头文件
file(GLOB_RECURSE SOURCES "src/*.cpp" "lib/*.cpp")
file(GLOB_RECURSE HEADERS "include/*.h")
link_directories(pthread)

# 添加可执行文件
add_executable(Server main.cpp ${SOURCES} ${HEADERS})
# 添加编译选项（如果需要）
# target_compile_options(MyExecutable PRIVATE -Wall -Wextra -pedantic)
