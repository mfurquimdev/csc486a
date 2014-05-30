cmake_minimum_required(VERSION 2.8)

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wstrict-aliasing=2 -fstrict-aliasing -std=c++11")
endif()

add_subdirectory(engine)
add_subdirectory(framework)
add_subdirectory(a1)
