#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "defs.h"
#include "token.h"
#include "vec.h"

#define test_assert(state, b)                                                  \
  if (!(b))                                                                    \
    test_fail_eq(state, #b, "true");

#define test_assert_eq(state, a, b)                                            \
  if ((a) != (b))                                                              \
    test_fail_eq(state, #a, #b);

#define test_assert_neq(state, a, b)                                           \
  if ((a) == (b))                                                              \
    test_fail_eq(state, #a, #b);

#define failf(state, ...) failf_(state, __FILE__, __LINE__, __VA_ARGS__)

typedef struct {
  bool junit;
  bool lite;
  const char *filter_str;
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
  struct timespec total_tokenization_time;
  uint64_t total_tokens;
#endif
#ifdef TIME_PARSER
  struct timespec total_parser_time;
  uint64_t total_tokens_parsed;
  uint64_t total_parse_nodes_produced;
#endif
#ifdef TIME_TYPECHECK
  struct timespec total_typecheck_time;
  uint64_t total_parse_nodes_typechecked;
#endif
#ifdef TIME_CODEGEN
  struct timespec total_llvm_ir_generation_time;
  struct timespec total_codegen_time;
  uint64_t total_parse_nodes_codegened;
#endif
  test_config config;
  vec_string path;
  uint32_t tests_passed;
  uint32_t tests_run;
  struct timespec start_time;
  struct timespec end_time;
  const char *current_name;
  u8 print_streaming : 1;
  u8 current_failed : 1;
  u8 in_test : 1;
  vec_failure failures;
  vec_test_action actions;
  vec_string strs;
  const char *filter_str;
} test_state;

#define test_start(state, desc)                                                \
  {                                                                            \
    const char *test_name = desc;                                              \
    if (test_matches(state, test_name)) {                                      \
      test_start_internal(state, test_name);

#define test_end(state)                                                        \
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
