#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "vec.h"

#define test_assert(state, b) \
  if (!(b)) test_fail_eq(state, #b, "true");

#define test_assert_eq(state, a, b) \
  if ((a) != (b)) test_fail_eq(state, #a, #b);

#define failf(state, ...) \
  failf_(state, __FILE__, __LINE__, __VA_ARGS__)

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
  vec_string path;
  uint32_t tests_passed;
  uint32_t tests_run;
  struct timespec start_time;
  struct timespec end_time;
  char *current_name;
  bool current_failed;
  bool in_test;
  vec_failure failures;
  bool junit;
  vec_test_action actions;
  vec_string strs;
  bool lite;
} test_state;

test_state test_state_new(void);
void test_state_finalize(test_state *state);

void test_group_end(test_state *state);
void test_group_start(test_state *state, char *name);

void test_end(test_state *state);
void test_start(test_state *state, char *name);

void failf_(test_state *state, char *file, size_t line, const char *fmt, ...);

void test_fail_eq(test_state *state, char *a, char *b);

void print_failures(test_state *state);

void test_vec(test_state *state);
void test_bitset(test_state *state);
void test_utils(test_state *state);
void test_scanner(test_state *state);
void test_parser(test_state *state);
void test_typecheck(test_state *state);

void write_test_results(test_state *state);
