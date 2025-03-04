cmake_minimum_required(VERSION 3.10)
project(max_nx)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_C_FLAGS "-Wall -O2 -march=armv8-a")
set(CMAKE_CXX_FLAGS "-Wall -O2 -march=armv8-a")

include_directories(include)
include_directories(${CMAKE_SOURCE_DIR})

file(GLOB SOURCES "source/*.c")

add_executable(max_nx ${SOURCES})