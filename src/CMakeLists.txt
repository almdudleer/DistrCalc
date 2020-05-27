cmake_minimum_required(VERSION 3.16)
project(pas C)

set(CMAKE_C_STANDARD 11)

include_directories(..)
include_directories(../inc)
include_directories(../inc/lib)

# pa1 libs
add_library(utils lib/utils.c ../inc/lib/utils.h)
add_library(ipc lib/ipc.c ../inc/lib/ipc.h ../inc/lib/ipc_utils.h lib/ipc_utils.c)
add_library(entity lib/entity.c ../inc/lib/entity.h)

# pa2 libs
add_library(banking ../inc/lib/banking.h ../inc/lib/lamp_time.h lib/lamp_time.c)

# executables
add_executable(pa2 ../inc/pa2345.h pa23.c)

target_link_libraries(pa2 utils ipc entity banking "/home/almdudleer/Studies/DistCalc/pas/libruntime.so")