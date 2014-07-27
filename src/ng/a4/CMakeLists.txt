cmake_minimum_required(VERSION 2.6)

project(a4 CXX)

file(GLOB_RECURSE SOURCES
    **.cpp)

include_directories(${NG_INCLUDE_DIR} ${NG_SRC_DIR})

set(ASSETS
    robotarm.obj
    robotarm.md5mesh
    robotarm.md5anim)

set_source_files_properties(${ASSETS} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(a4 ${SOURCES} ${ASSETS})
target_link_libraries(a4 engine framework)

foreach(assetFile ${ASSETS})
    add_custom_target("${assetFile}_export" ALL
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${CMAKE_CURRENT_SOURCE_DIR}/${assetFile} $<TARGET_FILE_DIR:a4>)
endforeach()
