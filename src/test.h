#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "parse_tree.h"
#include "token.h"
#include "typecheck.h"
#include "vec.h"

#define test_assert(state, b)                                                  \
  if (!(b))                                                                    \
    test_fail_eq(state, #b, "true");

#define test_assert_eq(state, a, b)                                            \
  if ((a) != (b))                                                              \
    test_fail_eq(state, #a, #b);

#define failf(state, ...) failf_(state, __FILE__, __LINE__, __VA_ARGS__)

typedef struct {
  bool junit;
  bool lite;
  char *filter_str;
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
  test_config config;
  vec_string path;
  uint32_t tests_passed;
  uint32_t tests_run;
  struct timespec start_time;
  struct timespec end_time;
  const char *current_name;
  bool current_failed;
  bool in_test;
  vec_failure failures;
  vec_test_action actions;
  vec_string strs;
  char *filter_str;
} test_state;

#define test_start(state, desc) { \
    const char *test_name = desc; \
    if (test_matches(state, test_name)) { \
      test_start_internal(state, test_name);

#define test_end(state) test_end_internal(state); } }

test_state test_state_new(test_config config);
void test_state_finalize(test_state *state);

void test_group_end(test_state *state);
void test_group_start(test_state *state, char *name);

void test_end_internal(test_state *state);
void test_start_internal(test_state *state, const char *name);

void failf_(test_state *state, char *file, size_t line, const char *fmt, ...);

void test_fail_eq(test_state *state, char *a, char *b);

void print_failures(test_state *state);

void test_vec(test_state *state);
void test_bitset(test_state *state);
void test_utils(test_state *state);
void test_scanner(test_state *state);
void test_parser(test_state *state);
void test_typecheck(test_state *state);
void test_llvm(test_state *state);

void write_test_results(test_state *state);

tokens_res test_upto_tokens(test_state *state, const char *input);
parse_tree_res test_upto_parse_tree(test_state *state, const char *input);
tc_res test_upto_typecheck(test_state *state, const char *input, bool *success,
                           parse_tree *tree);
bool test_matches(const test_state *restrict state, const char *restrict test_name);
