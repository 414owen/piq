#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

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
#ifdef JUNIT
  VEC_PUSH(&state->actions, GROUP_ENTER);
  VEC_PUSH(&state->strs, name);
#endif
}

void test_group_end(test_state *state) {
  VEC_POP_(&state->path);
#ifdef JUNIT
  VEC_PUSH(&state->actions, GROUP_LEAVE);
#endif
}

static vec_string make_test_path(test_state *state) {
  vec_string path = VEC_CLONE(&state->path);
  VEC_PUSH(&path, state->current_name);
  return path;
}

static char *ne_reason(char *a_name, char *b_name) {
  char *res;
  asprintf(&res, "Assert failed: '%s' != '%s'", a_name, b_name);
  return res;
}

// Static to avoid someone failing with a non-alloced string
static void fail_with(test_state *state, char *reason) {
  state->current_failed = true;
  failure f = {.path = make_test_path(state), .reason = reason};
  VEC_PUSH(&state->failures, f);
#ifdef JUNIT
  VEC_PUSH(&state->actions, TEST_FAIL);
#endif
}

void test_fail_eq(test_state *state, char *a, char *b) {
  fail_with(state, ne_reason(a, b));
}

void test_start(test_state *state, char *name) {
  print_depth_indent(state);
  fputs(name, stdout);
  state->current_name = name;
  state->current_failed = false;
#ifdef JUNIT
  VEC_PUSH(&state->actions, TEST_ENTER);
  VEC_PUSH(&state->strs, name);
#endif
}

void test_end(test_state *state) {
  printf(" %s" RESET "\n", state->current_failed ? RED "❌" : GRN "✓");
  if (!state->current_failed)
    state->tests_passed++;
  state->tests_run++;
#ifdef JUNIT
  VEC_PUSH(&state->actions, TEST_LEAVE);
#endif
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
    .start_time = 0,
    .end_time = 0,
    .current_name = NULL,
    .current_failed = false,
    .failures = VEC_NEW,
  };
  clock_gettime(CLOCK_MONOTONIC, &res.start_time);
  return res;
}

void test_state_finalize(test_state *state) {
  clock_gettime(CLOCK_MONOTONIC, &state->end_time);
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

#ifdef JUNIT

typedef struct {
  unsigned int tests;
  unsigned int failures;
} test_aggregate;

VEC_DECL(test_aggregate);

void write_test_results(test_state *state) {
  FILE *f = fopen("test-results.xml", "w");

  bool failed = false;
  vec_test_aggregate agg_stack = VEC_NEW;
  vec_test_aggregate aggs = VEC_NEW;
  test_aggregate current = {.tests = 0, .failures = 0};

  for (size_t i = 0; i < state->actions.len; i++) {
    test_action action = state->actions.data[state->actions.len - 1 - i];
    switch (action) {
      case GROUP_LEAVE:
        VEC_PUSH(&agg_stack, current)
        current.tests = 0;
        current.failures = 0;
        break;
      case GROUP_ENTER: {
        test_aggregate inner = VEC_POP(&agg_stack);
        VEC_PUSH(&aggs, current);
        current.tests += inner.tests;
        current.failures += inner.failures;
        break;
      }
      case TEST_LEAVE:
        current.tests++;
        failed = false;
        break;
      case TEST_FAIL:
        failed = true;
        break;
      case TEST_ENTER:
        if (failed)
          current.failures++;
        failed = false;
        break;
    }
  }

  VEC_FREE(&agg_stack);

  struct timespec elapsed;
  timespec_subtract(&elapsed, &state->end_time, &state->start_time);

  fprintf(f,
          "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          "<testsuites name=\"Unit tests\" tests=\"%u\" disabled=\"0\" "
          "errors=\"0\" failures=\"%u\" time=\"%lld\">\n",
          current.tests, current.failures, (long long)elapsed.tv_sec);

  size_t agg_ind = aggs.len - 1;
  size_t str_ind = 0;
  size_t fail_ind = 0;
  size_t depth = 1;

  for (size_t i = 0; i < state->actions.len; i++) {
    test_action action = state->actions.data[i];
    switch (action) {
      case GROUP_LEAVE:
        depth--;
      default:
        break;
    }
    for (size_t j = 0; j < depth; j++)
      fputs("  ", f);
    switch (action) {
      case GROUP_ENTER: {
        test_aggregate agg = aggs.data[agg_ind--];
        fprintf(f, "<testsuite name=\"%s\" tests=\"%u\" failures=\"%u\">\n",
                state->strs.data[str_ind++], agg.tests, agg.failures);
        depth++;
        break;
      }
      case TEST_ENTER:
        fprintf(f, "<testcase name=\"%s\" classname=\"\">\n",
                state->strs.data[str_ind++]);
        break;
      case TEST_LEAVE:
        fputs("</testcase>\n", f);
        break;
      case GROUP_LEAVE:
        fputs("</testsuite>\n", f);
        break;
      case TEST_FAIL:
        fprintf(f, "<failure message=\"Assertion failure\">%s</failure>\n",
                state->failures.data[fail_ind++].reason);
        break;
    }
  }
  fputs("</testsuites>\n", f);

  fflush(f);
  VEC_FREE(&aggs);
}
#endif
