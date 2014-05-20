cmake_minimum_required(VERSION 2.8)

set(NG_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(NG_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include)

add_subdirectory(src)

# Add headers to project for completeness
file(GLOB_RECURSE INTERFACE_FILES
    include/**.hpp include/**.inl)

add_library(ngPlaceholderHeaderLib STATIC ${INTERFACE_FILES})
set_target_properties(ngPlaceholderHeaderLib PROPERTIES LINKER_LANGUAGE CXX)

