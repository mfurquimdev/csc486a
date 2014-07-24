cmake_minimum_required(VERSION 2.6)

project(a3 CXX)

file(GLOB_RECURSE SOURCES
    **.cpp)

set(ASSETS
    bunny.obj
    cube.obj
    teapot.obj
    donut.obj)

set_source_files_properties(${ASSETS} PROPERTIES HEADER_FILE_ONLY TRUE)

include_directories(${NG_INCLUDE_DIR} ${NG_SRC_DIR})

add_executable(a3 ${SOURCES} ${ASSETS})
target_link_libraries(a3 engine framework)

foreach(assetFile ${ASSETS})
    add_custom_command(TARGET a3 POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${CMAKE_CURRENT_SOURCE_DIR}/${assetFile} $<TARGET_FILE_DIR:a3>)
endforeach()
