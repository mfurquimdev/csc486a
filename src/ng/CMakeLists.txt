cmake_minimum_required(VERSION 2.8)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_GNU_CXX_FLAGS "-Wall -Wextra -Wstrict-aliasing=2 -fstrict-aliasing -std=c++11")
    set(CMAKE_CXX_FLAGS ${CMAKE_GNU_CXX_FLAGS} ${CMAKE_CXX_FLAGS})
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CLANG_CXX_FLAGS "-stdlib=libc++ -std=c++11")
    set(CMAKE_CXX_FLAGS ${CMAKE_CLANG_CXX_FLAGS} ${CMAKE_CXX_FLAGS})
endif()

option(NG_ENABLE_PROFILING "Add flags for profiling with gprof" OFF)

if(NG_ENABLE_PROFILING)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif()

add_subdirectory(engine)
add_subdirectory(framework)
add_subdirectory(a2)
add_subdirectory(a4)
