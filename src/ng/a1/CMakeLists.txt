cmake_minimum_required(VERSION 2.6)

project(a1 CXX)

set(SOURCES
    main.cpp)

include_directories(${NG_INCLUDE_DIR} ${NG_SRC_DIR})

add_executable(a1 ${SOURCES})
target_link_libraries(a1 engine framework)
