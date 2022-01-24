#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#include "term.h"
#include "test.h"
#include "util.h"
#include "vec.h"

static const int test_indent = 2;

static void print_depth_indent(test_state *state) {
  for (int i = 0; i < state->path.len * test_indent; i++) {
    putc(' ', stdout);
  }
}

void test_group_start(test_state *state, char *name) {
  print_depth_indent(state);
  printf("%s\n", name);
  VEC_PUSH(&state->path, name);
}

void test_group_end(test_state *state) { VEC_POP_(&state->path); }

static vec_string make_test_path(test_state *state) {
  vec_string path = VEC_CLONE(&state->path);
  VEC_PUSH(&path, state->current_name);
  return path;
}

static char *ne_reason(char *a_name, char *b_name) {
  char *res;
  asprintf(&res, "Assert failed: %s != %s", a_name, b_name);
  return res;
}

// Static to avoid someone failing with a non-alloced string
static void fail_with(test_state *state, char *reason) {
  state->current_failed = true;
  failure f = {.path = make_test_path(state), .reason = reason};
  VEC_PUSH(&state->failures, f);
}

void test_fail_eq(test_state *state, char *a, char *b) {
  fail_with(state, ne_reason(a, b));
}

void test_start(test_state *state, char *name) {
  print_depth_indent(state);
  fputs(name, stdout);
  state->current_name = name;
  state->current_failed = false;
}

void test_end(test_state *state) {
  printf(" %s" RESET "\n", state->current_failed ? RED "❌" : GRN "✓");
}

void failf(test_state *state, const char *restrict fmt, ...) {
  char *err;
  va_list ap;
  va_start(ap, fmt);
  vasprintf(&err, fmt, ap);
  fail_with(state, err);
  va_end(ap);
}

test_state test_state_new(void) {
  test_state res = {
    .path = VEC_NEW,
    .tests_passed = 0,
    .tests_run = 0,
    .current_name = NULL,
    .current_failed = false,
    .failures = VEC_NEW,
  };
  return res;
}

static void print_failure(failure f) {
  fputs("\n" RED "FAILED" RESET ": ", stdout);
  for (size_t i = 0; i < f.path.len; i++) {
    if (i != 0)
      putc('/', stdout);
    fputs(f.path.data[i], stdout);
  }
  putc('\n', stdout);
  puts(f.reason);
}

void print_failures(test_state *state) {
  for (size_t i = 0; i < state->failures.len; i++) {
    print_failure(state->failures.data[i]);
  }
}
