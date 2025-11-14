cmake_minimum_required(VERSION 4.1.1)

set(PROJECT_NAME epollserv)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} main.cpp)