cmake_minimum_required(VERSION 2.6)

project(engine CXX)

find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

file(GLOB COMMON_SOURCES
    *.cpp *.hpp)

file(GLOB_RECURSE X11_SOURCES
    x11/**.cpp x11/**.hpp)

if(UNIX AND NOT APPLE)
    set(SOURCES
        ${COMMON_SOURCES}
        ${X11_SOURCES})
endif()

include_directories(${NG_INCLUDE_DIR} ${NG_SRC_DIR})

add_library(engine ${SOURCES})
target_link_libraries(engine ${X11_LIBRARIES} ${OPENGL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
