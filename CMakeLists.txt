cmake_minimum_required(VERSION 3.15)
project(pa1 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_COMPILER /usr/bin/clang)

add_library(ipc ipc.c ipc.h)
add_library(utils utils.h utils.c)
add_executable(pa1 pa1.h pa1.c utils.h utils.c ipc.h ipc.c Self.h Self.c)