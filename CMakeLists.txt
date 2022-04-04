cmake_minimum_required(VERSION 2.6)
project(vertex)

if (TARGET ${PROJECT_NAME})
	return()
endif()

add_subdirectory(ring)

add_library(${PROJECT_NAME}inc INTERFACE)
target_include_directories(${PROJECT_NAME}inc INTERFACE ./)

add_library(${PROJECT_NAME} vertex.c)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}inc ring pthread)

if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
	add_executable(test test.c)
	target_link_libraries(test ${PROJECT_NAME})
endif()
