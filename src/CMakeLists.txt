cmake_minimum_required(VERSION 3.16)
project(pas C)

set(CMAKE_C_STANDARD 11)

include_directories(..)
include_directories(../inc)
include_directories(../inc/lib)

add_library(utils lib/utils.c ../inc/lib/utils.h)
add_library(ipc lib/ipc.c ../inc/lib/ipc.h)
add_library(entity lib/entity.c ../inc/lib/entity.h)
add_library(lamport lib/lamp_time.c ../inc/lib/lamp_time.h)
add_library(queue lib/queue.c ../inc/lib/queue.h)

# executables
add_executable(pa4 ../inc/pa2345.h pa4.c)

target_link_libraries(pa4 utils ipc entity queue lamport "/home/almdudleer/Studies/DistCalc/pas/libruntime.so")