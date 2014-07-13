cmake_minimum_required(VERSION 2.6)

project(a3 CXX)

file(GLOB_RECURSE SOURCES
    **.cpp)

include_directories(${NG_INCLUDE_DIR} ${NG_SRC_DIR})

add_executable(a3 ${SOURCES})
target_link_libraries(a3 engine framework)
