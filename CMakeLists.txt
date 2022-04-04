cmake_minimum_required(VERSION 3.21)
project(vertex)

if (TARGET ${PROJECT_NAME})
    return()
endif ()

add_library(${PROJECT_NAME}inc INTERFACE)
target_include_directories(${PROJECT_NAME}inc INTERFACE include)

add_subdirectory(ring)
add_subdirectory(src)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_subdirectory(test)

endif ()
