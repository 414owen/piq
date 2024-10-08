// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "defs.h"
#include "timing.h"
#include "token.h"
#include "vec.h"

#define test_assert(state, b)                                                  \
  if (!(b))                                                                    \
    test_fail_eq(state, #b, "true");

#define test_assert_eq(state, a, b)                                            \
  if ((a) != (b))                                                              \
    test_fail_eq(state, #a, #b);

#define test_assert_eq_fmt(state, a, b, fmt)                                   \
  if ((a) != (b)) {                                                            \
    failf(state, #a " != " #b "\nExpected: '" fmt "', got: '" fmt "'", a, b);  \
  }

#define test_assert_eq_fmt_f(state, a, b, fmt, transform)                      \
  if ((a) != (b)) {                                                            \
    failf(state,                                                               \
          #a " != " #b "\nExpected: '" fmt "', got: '" fmt "'",                \
          (transform)(a),                                                      \
          (transform)(b));                                                     \
  }

#define test_assert_eq_fmt_a(state, a, b, fmt, transform)                      \
  if ((a) != (b)) {                                                            \
    failf(state,                                                               \
          #a " != " #b "\nExpected: '" fmt "', got: '" fmt "'",                \
          (transform)[a],                                                      \
          (transform)[b]);                                                     \
  }

#define test_assert_neq(state, a, b)                                           \
  if ((a) == (b))                                                              \
    test_fail_eq(state, #a, #b);

#define failf(state, ...) failf_(state, __FILE__, __LINE__, __VA_ARGS__)

typedef struct {
  bool junit;
  bool bench;
  bool lite;
  int times;
  char *filter_str;
  bool write_json;
} test_config;

typedef struct {
  vec_string path;
  char *reason;
} failure;

VEC_DECL(failure);

typedef enum {
  GROUP_ENTER,
  TEST_ENTER,
  GROUP_LEAVE,
  TEST_LEAVE,
  TEST_FAIL,
} test_action;

VEC_DECL(test_action);

typedef struct {
#ifdef TIME_TOKENIZER
  uint64_t total_bytes_tokenized;
  perf_values total_tokenization_perf;
  uint64_t total_tokens_produced;
#endif
#ifdef TIME_PARSER
  perf_values total_parser_perf;
  uint64_t total_tokens_parsed;
  uint64_t total_parse_nodes_produced;
#endif
#ifdef TIME_NAME_RESOLUTION
  perf_values total_name_resolution_perf;
  uint64_t total_names_looked_up;
#endif
#ifdef TIME_TYPECHECK
  perf_values total_typecheck_perf;
  uint64_t total_parse_nodes_typechecked;
#endif
#ifdef TIME_CODEGEN
  perf_values total_llvm_ir_generation_perf;
  perf_values total_codegen_perf;
  uint64_t total_parse_nodes_codegened;
#endif
  test_config config;
  vec_string path;
  uint32_t tests_passed;
  uint32_t tests_run;
  timespec start_time;
  timespec end_time;
  const char *current_name;
  u8 current_failed : 1;
  u8 in_test : 1;
  vec_failure failures;
  vec_test_action actions;
  vec_string strs;
  const char *filter_str;
} test_state;

#define test_start(state, desc)                                                \
  {                                                                            \
    const char *__test_name = desc;                                            \
    if (test_matches(state, __test_name)) {                                    \
      test_start_internal(state, __test_name);                                 \
      for (int __i = 0; __i < state->config.times; __i++) {

#define test_end(state)                                                        \
  }                                                                            \
  test_end_internal(state);                                                    \
  }                                                                            \
  }

test_state test_state_new(test_config config);
void test_state_finalize(test_state *state);

void test_group_end(test_state *state);
void test_group_start(test_state *state, char *name);

void test_end_internal(test_state *state);
void test_start_internal(test_state *state, const char *name);

void failf_(test_state *state, char *file, size_t line, const char *fmt, ...);

void test_fail_eq(test_state *state, char *a, char *b);

void print_failures(test_state *state);

void write_test_results(test_state *state);
bool test_matches(const test_state *state, const char *test_name);
