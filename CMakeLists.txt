cmake_minimum_required(VERSION 3.0)

set(CMAKE_COLOR_DIAGNOSTICS ON)

project(piq C CXX)

enable_testing()

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -s -flto")

add_subdirectory(src)
