cmake_minimum_required(VERSION 3.9)

set(CMAKE_COLOR_DIAGNOSTICS ON)


project(piq C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

if (DEFINED CI)
  add_definitions(-DBUILD_FOR_CI)
endif()

if (MSVC)
    # warning level 4
    add_compile_options(/W4)
else()
    # additional warnings
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-O0 -Wno-error=cpp)
endif()

if (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-s -fomit-frame-pointer)
  endif()
  add_compile_options(-ffunction-sections -fdata-sections)
  if (CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_link_options(-Wl,-dead_strip)
  else()
    add_link_options(-Wl,--gc-sections)
  endif()

endif()

add_compile_options(-Wall -pedantic)

message("Welcome to the piq build system")

message("  C compiler: '${CMAKE_C_COMPILER}'")
message("C++ compiler: '${CMAKE_CXX_COMPILER}'")

set (COMMON_OBJS
  src/bitset.c
  src/consts.c
  src/diagnostic.c
  src/mkdir_p.c
  src/dir_exists.c
  src/parser.c
  src/parse_tree.c
  src/builtins.c
  src/resolve_scope.c
  src/strint.c
  src/tokenizer.c
  src/token.c
  src/typecheck.c
  src/type.c
  src/util.c
  src/vec.c
  src/perf.c
  src/platform/linux/perf.c
  src/args.c
  src/timespec.c
  src/timing.c
  src/traverse.c
  src/initialise.c
  src/types.c
  src/print_tc_errors.c
  src/hashmap.c
  src/hashers.c
  src/global_settings.c
  src/log.c
  # src/llvm_text.c
  src/externalise_spans.c
  src/reorder_tree.c
)

set (TEST_OBJS
  src/run_tests.c
  src/span.c
  src/test_bitset.c
  src/benchmark.c
  src/test.c
  src/test_parser.c
  src/test_diagnostics.c
  src/test_scanner.c
  src/test_semantics.c
  src/test_strint.c
  src/test_typecheck.c
  src/test_traverse.c
  src/test_upto.c
  src/test_utils.c
  src/test_vec.c
  src/test_hashmap.c
)

set (COMPILE_FOR_SIZE
  src/test_parser.c
  src/test_typecheck.c
  src/test_scanner.c
  src/test.c
  src/run_tests.c
  src/test_bitset.c
  src/test_diagnostics.c
  src/benchmark.c
  src/test_semantics.c
  src/test_strint.c
  src/test_utils.c
  src/test_vec.c
  src/test_upto.c
  src/test_traverse.c
  src/test_hashmap.c
  src/timing.c
  src/perf.c
  src/platform/linux/perf.c
  src/timespec.c
  src/token.c
  src/args.c
  src/main.c
  src/builtins.c
  src/initialise.c
  src/print_tc_errors.c
  src/global_settings.c
  src/log.c
)

# -----------
# Set up LLVM
# -----------

find_package(LLVM CONFIG)

if(LLVM_FOUND)

  message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
  message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

  include_directories(${LLVM_INCLUDE_DIRS})
  separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
  add_definitions(${LLVM_DEFINITIONS_LIST})

  set (LLVM_COMPILE_LINK_COMPONENTS
    Core
    ExecutionEngine
    MC
    nativecodegen
    mcjit
    asmparser
    native
  )

   set (LLVM_JIT_LINK_COMPONENTS
    Core
    ExecutionEngine
    MC
    core
    orcjit
    executionengine
    mcjit
    analysis
    native
    bitwriter
  )

  list(APPEND COMMON_OBJS
    src/llvm/construct_type.c
    src/llvm.c
    src/llvm_shim.cpp
  )

  list(APPEND TEST_OBJS
    src/llvm_test_utils.c
    src/test_llvm.c
  )

  list(APPEND COMPILE_FOR_SIZE
    src/test_llvm.c
    src/llvm_test_utils.c
  )

endif()

# ---------
# Run build
# ---------

# add_executable(test run_tests.c)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.h
         ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.c
  COMMAND
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen-parser.sh
      ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.y
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen-parser.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.y
)

add_custom_command(
  OUTPUT 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tokenizer.c
  COMMAND re2c
    -o ${CMAKE_CURRENT_SOURCE_DIR}/src/tokenizer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tokenizer.re
  # lemon generates header with tokens
  # Not a good way of doing it tbh
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tokenizer.re
)

set (MAIN_OBJS
  src/main.c
  src/repl.c
)

foreach (file ${COMPILE_FOR_SIZE})
  set_source_files_properties(${file} PROPERTIES COMPILE_FLAGS -Os)
endforeach(file)


add_executable(piq ${MAIN_OBJS} ${COMMON_OBJS})
add_executable(test-exe ${TEST_OBJS} ${COMMON_OBJS})


if(LLVM_FOUND)
  llvm_config(piq USE_SHARED ${LLVM_COMPILE_LINK_COMPONENTS})
  llvm_config(test-exe USE_SHARED ${LLVM_JIT_LINK_COMPONENTS})
endif()


cmake_policy(SET CMP0069 NEW) 
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

include(CheckIPOSupported)
check_ipo_supported(RESULT IPOSupported OUTPUT IPOError)

if(IPOSupported)
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET piq test-exe PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${IPOError}>")
endif()

target_link_libraries(piq PRIVATE edit)

foreach(exe piq)
  set_property(TARGET ${exe} PROPERTY C_STANDARD 99)
endforeach(exe)

include(GNUInstallDirs)

add_test(NAME test COMMAND test-exe bench)

if(DEFINED INSTALL_TEST)
  set(TEST_INSTALL_TARGET test-exe)
endif()

# For testing 32-bit compilation
# set_target_properties(test-exe piq PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")

install(
  TARGETS piq ${TEST_INSTALL_TARGET}
  CONFIGURATIONS Release
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
