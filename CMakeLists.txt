project(luemiu)

cmake_minimum_required(VERSION 2.8)

include_directories(include)

aux_source_directory(./src SRCS)

add_executable(luemiu ${SRCS})

target_link_libraries(luemiu pthread)
