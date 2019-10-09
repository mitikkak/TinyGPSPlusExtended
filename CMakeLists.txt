cmake_minimum_required(VERSION 3.1)

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR ${PROJECT_ROOT}/TinyGPSPlus/src)

add_subdirectory(${PROJECT_ROOT}/googletest)
#Run cmake with -DCMAKE_BUILD_TYPE=Debug
add_subdirectory(${PROJECT_ROOT}/cmake/ut)
