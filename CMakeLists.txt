project(luemiu)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-DDEBUG)

link_libraries(ssl crypto pthread)

include_directories(include)

aux_source_directory(./src SRCS)

add_executable(luemiu ${SRCS})

target_link_libraries(luemiu pthread)
