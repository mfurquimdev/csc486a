cmake_minimum_required(VERSION 2.6)

project(engine CXX)

option(NG_USE_X11 "Use X11 for window creation" ON)
option(NG_USE_EGL "Use EGL for OpenGL context creation" OFF)
option(NG_USE_EMSCRIPTEN "Build with the quirks necessary to run the app in Emscripten" OFF)

if(NG_USE_EMSCRIPTEN)
    if(NOT NG_USE_EGL)
        message("Enabling EGL since we're compiling for Emscripten")
        set(NG_USE_EGL ON)
    endif()
endif()

set(COMMON_DIRS
    .
    util
    window
    rendering
    filesystem)

foreach(common_dir ${COMMON_DIRS})
    file(GLOB sources
        ${common_dir}/*.cpp
        ${common_dir}/*.hpp)
    set(COMMON_SOURCES ${COMMON_SOURCES} ${sources})
endforeach()

file(GLOB_RECURSE X11_SOURCES
    x11/**.cpp x11/**.hpp)

file(GLOB_RECURSE EGL_SOURCES
    egl/**.cpp egl/**.hpp)

file(GLOB_RECURSE EMSCRIPTEN_SOURCES
    emscripten/**.cpp emscripten/**.hpp)

file(GLOB_RECURSE OPENGL_SOURCES
    opengl/**.cpp opengl/**.hpp)

set(SOURCES ${COMMON_SOURCES})

set(LIBS "")

find_package(OpenGL)
set(LIBS ${LIBS} ${OPENGL_LIBRARIES})

# emscripten does not support pthreads. :/
if(NOT NG_USE_EMSCRIPTEN)
    find_package(Threads REQUIRED)
    set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif()

set(SOURCES ${SOURCES} ${OPENGL_SOURCES})

if(NG_USE_X11)
    find_package(X11)
    set(SOURCES ${SOURCES} ${X11_SOURCES})
    set(LIBS ${LIBS} ${X11_LIBRARIES})
    add_definitions(-DNG_USE_X11)
else()
    set(UNUSED_SOURCES ${UNUSED_SOURCES} ${X11_SOURCES})
endif()

if(NG_USE_EGL)
    set(SOURCES ${SOURCES} ${EGL_SOURCES})
    add_definitions(-DNG_USE_EGL)
else()
    set(UNUSED_SOURCES ${UNUSED_SOURCES} ${EGL_SOURCES})
endif()

if(NG_USE_EMSCRIPTEN)
    set(SOURCES ${SOURCES} ${EMSCRIPTEN_SOURCES})
    add_definitions(-DNG_USE_EMSCRIPTEN)
else()
    set(UNUSED_SOURCES ${UNUSED_SOURCES} ${EMSCRIPTEN_SOURCES})
endif()

include_directories(${NG_INCLUDE_DIR} ${NG_SRC_DIR})

set_source_files_properties(${UNUSED_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_library(engine ${SOURCES} ${UNUSED_SOURCES})
target_link_libraries(engine ${LIBS})
