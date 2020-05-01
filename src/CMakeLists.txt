cmake_minimum_required(VERSION 3.16)
project(pas C)

set(CMAKE_C_STANDARD 11)

include_directories(..)
include_directories(../inc)
include_directories(../inc/lib)

# pa1 libs
add_library(utils lib/utils.c ../inc/lib/utils.h)
add_library(ipc lib/ipc.c ../inc/lib/ipc.h)
add_library(self lib/self.c ../inc/lib/self.h)

# pa2 libs
add_library(banking lib/banking.c lib/bank_robbery.c ../inc/lib/banking.h)

# executables
add_executable(pa1 ../inc/pa1.h pa1.c ../inc/common.h)
add_executable(pa2 ../inc/pa2345.h pa23.c)

target_link_libraries(pa1 utils ipc self)
target_link_libraries(pa2 utils ipc self banking "/home/almdudleer/Studies/DistCalc/pas/libruntime.so")