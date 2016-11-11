cmake_minimum_required(VERSION 3.6)
project(rbTree)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


set(SOURCE_FILES
    demo.cpp
    rb_tree.h)

add_executable(rbTree ${SOURCE_FILES})