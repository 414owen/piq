#pragma once

#include <stdbool.h>

#include "vec.h"

#define test_assert_eq(state, a, b) \
  if ((a) != (b)) test_fail_eq(state, #a, #b);

typedef struct {
  vec_string path;
  char *reason;
} failure;

VEC_DECL(failure);

typedef struct {
  vec_string path;
  unsigned tests_passed;
  unsigned tests_run;
  char *current_name;
  bool current_failed;
  vec_failure failures;
} test_state;

test_state test_state_new(void);

void test_group_end(test_state *state);
void test_group_start(test_state *state, char *name);

void test_end(test_state *state);
void test_start(test_state *state, char *name);

void failf(test_state *state, const char *restrict fmt, ...);

void test_fail_eq(test_state *state, char *a, char *b);

void print_failures(test_state *state);

void test_utils(test_state *state);
void test_scanner(test_state *state);
void test_parser(test_state *state);
