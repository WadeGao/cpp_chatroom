cmake_minimum_required(VERSION 3.10.1)
project(IM)
add_executable(Client Common.hpp Client.hpp Client.cpp ClientMain.cpp)
add_executable(Server Common.hpp Server.hpp Server.cpp ServerMain.cpp)