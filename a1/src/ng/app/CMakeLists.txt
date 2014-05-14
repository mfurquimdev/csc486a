cmake_minimum_required(VERSION 2.6)

project(app CXX)

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

set(SOURCES
    main.cpp)

include_directories(${NG_DIRECTORY})

add_executable(app ${SOURCES})
target_link_libraries(app engine)
