cmake_minimum_required(VERSION 3.0)

add_compile_options(-Wall -pedantic)

message("Welcome to the piq build system")

message("  C compiler: '${CMAKE_C_COMPILER}'")
message("C++ compiler: '${CMAKE_CXX_COMPILER}'")

# -----------
# Set up LLVM
# -----------

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# ---------
# Run build
# ---------

# add_executable(test run_tests.c)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/parser.h
         ${CMAKE_CURRENT_SOURCE_DIR}/parser.c
  COMMAND
    ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/gen-parser.sh ${CMAKE_CURRENT_SOURCE_DIR}/parser.y ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_command(
  OUTPUT 
    ${CMAKE_CURRENT_SOURCE_DIR}/tokenizer.c
  COMMAND re2c
    -o ${CMAKE_CURRENT_SOURCE_DIR}/tokenizer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/tokenizer.re
  # lemon generates header with tokens
  # Not a good way of doing it tbh
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/parser.h
)

set (COMMON_OBJS
  bitset.c
  consts.c
  llvm/construct_type.c
  diagnostic.c
  dir_exists.c
  mkdir_p.c
  parser.c
  parse_tree.c
  builtins.c
  resolve_scope.c
  strint.c
  tokenizer.c
  token.c
  typecheck.c
  type.c
  util.c
  vec.c
  llvm.c
  llvm_shim.cpp
  perf.c
  platform/linux/perf.c
  args.c
  timespec.c
  timing.c
  traverse.c
  initialise.c
  types.c
  print_tc_errors.c
  hashmap.c
  hashers.c
  global_settings.c
  log.c
  externalise_spans.c
)

set (MAIN_OBJS
  main.c
  repl.c
)

set (TEST_OBJS
  run_tests.c
  span.c
  test_bitset.c
  benchmark.c
  llvm_test_utils.c
  test.c
  test_llvm.c
  test_parser.c
  test_diagnostics.c
  test_parse_tree.c
  test_scanner.c
  test_semantics.c
  test_strint.c
  test_typecheck.c
  test_traverse.c
  test_upto.c
  test_utils.c
  test_vec.c
  test_hashmap.c
)

set (COMPILE_FOR_SIZE
  test_llvm.c
  test_parser.c
  test_typecheck.c
  test_scanner.c
  test.c
  run_tests.c
  test_bitset.c
  test_diagnostics.c
  benchmark.c
  llvm_test_utils.c
  test_semantics.c
  test_strint.c
  test_utils.c
  test_vec.c
  test_upto.c
  test_parse_tree.c
  test_traverse.c
  test_hashmap.c
  timing.c
  perf.c
  platform/linux/perf.c
  timespec.c
  token.c
  args.c
  main.c
  builtins.c
  initialise.c
  print_tc_errors.c
  global_settings.c
  log.c
)

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

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_compile_libs ${LLVM_COMPILE_LINK_COMPONENTS})
llvm_map_components_to_libnames(llvm_jit_libs ${LLVM_JIT_LINK_COMPONENTS})

add_executable(piq ${MAIN_OBJS} ${COMMON_OBJS})
add_executable(test-exe ${TEST_OBJS} ${COMMON_OBJS})

message("LLVM args: ${llvm_jit_libs}")

target_link_libraries(piq ${llvm_compile_libs} readline)
target_link_libraries(test-exe ${llvm_jit_libs})

foreach(exe piq)
  set_property(TARGET ${exe} PROPERTY C_STANDARD 99)
endforeach(exe)

include(GNUInstallDirs)

add_test(NAME test COMMAND test-exe bench)

if(DEFINED INSTALL_TEST)
  set(TEST_INSTALL_TARGET test-exe)
endif()

install(
  TARGETS piq ${TEST_INSTALL_TARGET}
  CONFIGURATIONS Release
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

foreach (file ${COMPILE_FOR_SIZE})
endforeach(file)
