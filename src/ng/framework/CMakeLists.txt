cmake_minimum_required(VERSION 2.6)

project(framework CXX)

file(GLOB_RECURSE SOURCES
    **.cpp **.hpp)

include_directories(${NG_INCLUDE_DIR} ${NG_SRC_DIR})

add_library(framework ${SOURCES})
target_link_libraries(framework engine)
