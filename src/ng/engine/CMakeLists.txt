cmake_minimum_required(VERSION 2.6)

project(engine CXX)

option(NG_USE_X11 "Use X11 for window creation" ON)

find_package(OpenGL REQUIRED)

if(NG_USE_X11)
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
endif()

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++11")
endif()

file(GLOB COMMON_SOURCES
    *.cpp *.hpp)

file(GLOB_RECURSE X11_SOURCES
    x11/**.cpp x11/**.hpp)

file(GLOB_RECURSE OPENGL_SOURCES
    opengl/**.cpp opengl/**.hpp)

set(SOURCES ${COMMON_SOURCES})

set(LIBS ${OPENGL_LIBRARIES})

set(SOURCES ${SOURCES} ${OPENGL_SOURCES})

if(NG_USE_X11)
    set(SOURCES ${SOURCES} ${X11_SOURCES})
    set(LIBS ${LIBS} ${X11_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    add_definitions(-DNG_USE_X11)
else()
    message(FATAL_ERROR "Unsupported configuration")
endif()

include_directories(${NG_INCLUDE_DIR} ${NG_SRC_DIR})

add_library(engine ${SOURCES})
target_link_libraries(engine ${LIBS})
