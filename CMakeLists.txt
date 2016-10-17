cmake_minimum_required(VERSION 3.6)
project(cache_miss)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(cache_miss ${SOURCE_FILES})