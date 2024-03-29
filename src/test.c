// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <assert.h>
#include <hedley.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "diagnostic.h"
#include "term.h"
#include "test.h"
#include "timing.h"
#include "timespec.h"
#include "token.h"
#include "util.h"
#include "vec.h"

static const int test_indent = 2;

static void print_depth_indent(test_state *state) {
  for (uint32_t i = 0; i < state->path.len * test_indent; i++) {
    putc(' ', stdout);
  }
}

void test_group_start(test_state *state, char *name) {
  assert(!state->in_test);
  print_depth_indent(state);
  printf("%s\n", name);
  VEC_PUSH(&state->path, name);
  if (state->config.junit) {
    test_action a = GROUP_ENTER;
    VEC_PUSH(&state->actions, a);
    VEC_PUSH(&state->strs, name);
  }
}

void test_group_end(test_state *state) {
  assert(!state->in_test);
  VEC_POP_(&state->path);
  if (state->config.junit) {
    test_action a = GROUP_LEAVE;
    VEC_PUSH(&state->actions, a);
  }
}

static vec_string make_test_path(test_state *state) {
  vec_string path;
  VEC_CLONE(&path, &state->path);
  VEC_PUSH(&path, state->current_name);
  return path;
}

static char *ne_reason(char *a_name, char *b_name) {
  char *res;
  int amt = asprintf(&res, "Assert failed: '%s' != '%s'", a_name, b_name);
  (void)amt;
  return res;
}

// Static to avoid someone failing with a non-alloced string
static void fail_with(test_state *state, char *reason) {
  assert(state->in_test);
  state->current_failed = true;
  failure f = {.path = make_test_path(state), .reason = reason};
  VEC_PUSH(&state->failures, f);
  if (state->config.junit) {
    test_action act = TEST_FAIL;
    VEC_PUSH(&state->actions, act);
  }
}

void test_fail_eq(test_state *state, char *a, char *b) {
  fail_with(state, ne_reason(a, b));
}

void test_start_internal(test_state *state, const char *name) {
  assert(!state->in_test);
  assert(state->path.len > 0);
  print_depth_indent(state);
  fputs(name, stdout);
  putc(' ', stdout);
  // we want to know what test is being run when we crash
  fflush(stdout);
  state->current_name = name;
  state->current_failed = false;
  state->in_test = true;
  if (state->config.junit) {
    test_action a = TEST_ENTER;
    VEC_PUSH(&state->actions, a);
    VEC_PUSH(&state->strs, name);
  }
}

void test_end_internal(test_state *state) {
  printf("%s" RESET "\n", state->current_failed ? RED "✕" : GRN "✓");
  if (!state->current_failed)
    state->tests_passed++;
  state->tests_run++;
  state->in_test = false;
  if (state->config.junit) {
    test_action a = TEST_LEAVE;
    VEC_PUSH(&state->actions, a);
  }
}

HEDLEY_PRINTF_FORMAT(4, 5)
void failf_(test_state *state, char *file, size_t line, const char *fmt, ...) {
  stringstream ss;
  ss_init_immovable(&ss);
  fprintf(ss.stream, "In %s line %zu: ", file, line);

  va_list ap;
  va_start(ap, fmt);
  vfprintf(ss.stream, fmt, ap);
  va_end(ap);

  ss_finalize(&ss);
  fail_with(state, ss.string);
}

test_state test_state_new(test_config config) {
  test_state res = {
#ifdef TIME_TOKENIZER
    .total_bytes_tokenized = 0,
    .total_tokenization_perf = perf_zero,
    .total_tokens_produced = 0,
#endif
#ifdef TIME_PARSER
    .total_parser_perf = perf_zero,
    .total_tokens_parsed = 0,
    .total_parse_nodes_produced = 0,
#endif
#ifdef TIME_NAME_RESOLUTION
    .total_name_resolution_perf = perf_zero,
    .total_names_looked_up = 0,
#endif
#ifdef TIME_TYPECHECK
    .total_typecheck_perf = perf_zero,
    .total_parse_nodes_typechecked = 0,
#endif
#ifdef TIME_CODEGEN
    .total_llvm_ir_generation_perf = perf_zero,
    .total_codegen_perf = perf_zero,
    .total_parse_nodes_codegened = 0,
#endif
    .config = config,
    .path = VEC_NEW,
    .tests_passed = 0,
    .tests_run = 0,
    .current_name = NULL,
    .current_failed = false,
    .failures = VEC_NEW,
    .filter_str = config.filter_str,
  };
  return res;
}

void test_state_finalize(test_state *state) {
  state->end_time = get_monotonic_time();
}

static void print_test_path(vec_string v, FILE *out) {
  if (v.len == 0)
    return;
  fputs(VEC_GET(v, 0), out);
  for (size_t i = 1; i < v.len; i++) {
    putc('/', out);
    fputs(VEC_GET(v, i), out);
  }
}

static void print_failure(failure f) {
  fputs("\n" RED "FAILED" RESET ": ", stdout);
  print_test_path(f.path, stdout);
  putc('\n', stdout);
  puts(f.reason);
}

void print_failures(test_state *state) {
  for (size_t i = 0; i < state->failures.len; i++) {
    print_failure(VEC_GET(state->failures, i));
  }
}

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
    test_action action = VEC_GET(state->actions, state->actions.len - 1 - i);
    switch (action) {
      case GROUP_LEAVE:
        VEC_PUSH(&agg_stack, current)
        current.tests = 0;
        current.failures = 0;
        break;
      case GROUP_ENTER: {
        test_aggregate inner;
        VEC_POP(&agg_stack, &inner);
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
  vec_string class_path = VEC_NEW;

  difftime_t elapsed = timespec_subtract(state->end_time, state->start_time);

  fprintf(f,
          "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          "<testsuites name=\"Unit tests\" tests=\"%u\" disabled=\"0\" "
          "errors=\"0\" failures=\"%u\" time=\"%lld\">\n",
          current.tests,
          current.failures,
          difftime_to_secs(elapsed));

  size_t agg_ind = aggs.len - 1;
  size_t str_ind = 0;
  size_t fail_ind = 0;
  size_t depth = 1;

  for (size_t i = 0; i < state->actions.len; i++) {
    test_action action = VEC_GET(state->actions, i);
    switch (action) {
      case GROUP_LEAVE:
        depth--;
        VEC_POP_(&class_path);
        break;
      default:
        break;
    }

    for (size_t j = 0; j < depth; j++) {
      fputs("  ", f);
    }

    switch (action) {
      case GROUP_ENTER: {
        test_aggregate agg = VEC_GET(aggs, agg_ind);
        agg_ind--;
        char *str = VEC_GET(state->strs, str_ind);
        str_ind++;
        fprintf(f,
                "<testsuite name=\"%s\" tests=\"%u\" failures=\"%u\">\n",
                str,
                agg.tests,
                agg.failures);
        VEC_PUSH(&class_path, str);
        depth++;
        break;
      }
      case TEST_ENTER:
        fprintf(f,
                "<testcase name=\"%s\" classname=\"",
                VEC_GET(state->strs, str_ind));
        str_ind++;
        for (size_t i = 0; i < class_path.len; i++) {
          if (i > 0)
            putc('/', f);
          fputs(VEC_GET(class_path, i), f);
        }
        fputs("\">\n", f);
        break;
      case TEST_LEAVE:
        fputs("</testcase>\n", f);
        break;
      case GROUP_LEAVE:
        fputs("</testsuite>\n", f);
        break;
      case TEST_FAIL:
        fprintf(f,
                "<failure message=\"Assertion failure\">%s</failure>\n",
                VEC_GET(state->failures, fail_ind).reason);
        fail_ind--;
        break;
    }
  }
  fputs("</testsuites>\n", f);

  fflush(f);
  VEC_FREE(&aggs);
  VEC_FREE(&class_path);
}

bool test_matches(const test_state *restrict state,
                  const char *restrict test_name) {
  if (state->filter_str == NULL) {
    return true;
  }
  VEC_PUSH(&state->path, test_name);
  stringstream ss;
  ss_init_immovable(&ss);
  print_test_path(state->path, ss.stream);
  ss_finalize(&ss);
  VEC_POP_(&state->path);
  bool res = strstr(ss.string, state->filter_str) != NULL;
  free(ss.string);
  return res;
}
