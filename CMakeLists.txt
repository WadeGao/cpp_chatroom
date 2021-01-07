#cmake版本
cmake_minimum_required(VERSION 3.4.1)

#项目名：IM_Software
project(IM_Software)

# C++17 编译
set(CMAKE_CXX_STANDARD 14)

# 指定根目录
set(ROOT ${PROJECT_SOURCE_DIR})
#指定子模块源文件目录
set(MODULES ${PROJECT_SOURCE_DIR}/src)

# 可执行文件输出目录
set(EXECUTABLE_OUTPUT_PATH ${ROOT}/bin)

add_subdirectory(${MODULES}/Client)
add_subdirectory(${MODULES}/Server)
