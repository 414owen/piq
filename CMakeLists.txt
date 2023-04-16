cmake_minimum_required(VERSION 3.9)

set(CMAKE_COLOR_DIAGNOSTICS ON)

project(piq C CXX)

include(CheckIPOSupported)
check_ipo_supported(RESULT IPOSupported OUTPUT IPOError)

enable_testing()

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -s")

add_subdirectory(src)
