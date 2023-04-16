cmake_minimum_required(VERSION 3.9)

set(CMAKE_COLOR_DIAGNOSTICS ON)


project(piq C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

if (MSVC)
    # warning level 4
    add_compile_options(/W4)
else()
    # additional warnings
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

  if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-s -fomit-frame-pointer)
  endif()
  add_compile_options(-ffunction-sections -fdata-sections)
  add_link_options(-Wl,--gc-sections)

endif()

add_subdirectory(src)
