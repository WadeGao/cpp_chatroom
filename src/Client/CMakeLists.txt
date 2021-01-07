#cmake版本
cmake_minimum_required(VERSION 3.4.1)

#项目名：Client
project(Client)

# 指定根目录
set(ROOT ${PROJECT_SOURCE_DIR}/../../)

# 指定头文件路径
set(INC_DIR ${ROOT}/include)

#指定头文件目录
include_directories(${INC_DIR})

# 添加当前目录下所有的源文件
aux_source_directory(${ROOT}/src/Client CLIENT_ALL_SRCS)

add_executable(Client ${CLIENT_ALL_SRCS})
