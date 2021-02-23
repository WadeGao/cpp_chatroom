#cmake版本
cmake_minimum_required(VERSION 3.4.1)

#项目名：Server
project(Server)

# 指定根目录
set(ROOT ${PROJECT_SOURCE_DIR}/../../)

# 指定头文件路径
set(INC_DIR ${ROOT}/include)

#链接库，mysqlclient为库名
link_libraries(mysqlclient)

#指定头文件目录
include_directories(${INC_DIR})

# 添加当前目录下所有的源文件
aux_source_directory(${ROOT}/src/Server SERVER_ALL_SRCS)
aux_source_directory(${ROOT}/src/Common DATABASE_ALL_SRC)

add_executable(Server ${SERVER_ALL_SRCS} ${DATABASE_ALL_SRC})
