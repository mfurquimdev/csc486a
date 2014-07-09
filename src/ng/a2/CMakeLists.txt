cmake_minimum_required(VERSION 2.6)

project(a2 CXX)

file(GLOB_RECURSE SOURCES
    **.cpp)

include_directories(${NG_INCLUDE_DIR} ${NG_SRC_DIR})

add_executable(a2 ${SOURCES})
target_link_libraries(a2 engine framework)
