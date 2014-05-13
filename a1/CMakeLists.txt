cmake_minimum_required(VERSION 2.8)

project(a1 CXX)

file(GLOB_RECURSE SOURCES
    src/**.hpp src/**.cpp)

include_directories(src)

add_executable(a1 ${SOURCES})
