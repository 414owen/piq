#include <assert.h>
#include <hedley.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "diagnostic.h"
#include "parse_tree.h"
#include "term.h"
#include "test.h"
#include "token.h"
#include "util.h"
#include "vec.h"
#include "typecheck.h"

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
  asprintf(&res, "Assert failed: '%s' != '%s'", a_name, b_name);
  return res;
}

// Static to avoid someone failing with a non-alloced string
static void fail_with(test_state *state, char *reason) {
  assert(state->in_test);
  state->current_failed = true;
  failure f = {.path = make_test_path(state), .reason = reason};
  VEC_PUSH(&state->failures, f);
  if (state->config.junit) {
    VEC_PUSH(&state->actions, TEST_FAIL);
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
  printf(" %s" RESET "\n", state->current_failed ? RED "❌" : GRN "✓");
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
  stringstream *ss = ss_init();
  fprintf(ss->stream, "In %s line %zu: ", file, line);

  va_list ap;
  va_start(ap, fmt);
  vfprintf(ss->stream, fmt, ap);
  va_end(ap);

  char *err = ss_finalize_free(ss);
  fail_with(state, err);
}

test_state test_state_new(test_config config) {
  test_state res = {
    .config = config,
    .path = VEC_NEW,
    .tests_passed = 0,
    .tests_run = 0,
    .start_time = {0},
    .end_time = {0},
    .current_name = NULL,
    .current_failed = false,
    .failures = VEC_NEW,
    .filter_str = config.filter_str,
  };
  clock_gettime(CLOCK_MONOTONIC, &res.start_time);
  return res;
}

void test_state_finalize(test_state *state) {
  clock_gettime(CLOCK_MONOTONIC, &state->end_time);
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
  vec_string class_path = VEC_NEW;

  struct timespec elapsed =
    timespec_subtract(state->end_time, state->start_time);

  fprintf(f,
          "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
          "<testsuites name=\"Unit tests\" tests=\"%u\" disabled=\"0\" "
          "errors=\"0\" failures=\"%u\" time=\"%lld\">\n",
          current.tests,
          current.failures,
          (long long)elapsed.tv_sec);

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
    for (size_t j = 0; j < depth; j++)
      fputs("  ", f);
    switch (action) {
      case GROUP_ENTER: {
        test_aggregate agg = VEC_GET(aggs, agg_ind--);
        char *str = VEC_GET(state->strs, str_ind++);
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
                VEC_GET(state->strs, str_ind++));
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
                VEC_GET(state->failures, fail_ind++).reason);
        break;
    }
  }
  fputs("</testsuites>\n", f);

  fflush(f);
  VEC_FREE(&aggs);
  VEC_FREE(&class_path);
}

tokens_res test_upto_tokens(test_state *state, const char *restrict input) {
  source_file test_file = {.path = "parser-test", .data = input};
  tokens_res tres = scan_all(test_file);
  if (!tres.succeeded) {
    stringstream *ss = ss_init();
    format_error_ctx(ss->stream, input, tres.error_pos, tres.error_pos);
    failf(state, "Scanning failed:\n%s", ss_finalize_free(ss));
    free_tokens_res(tres);
  }
  return tres;
}

parse_tree_res test_upto_parse_tree(test_state *state,
                                    const char *restrict input) {
  tokens_res tres = test_upto_tokens(state, input);
  if (!tres.succeeded) {
    parse_tree_res res = {
      .succeeded = false,
      .error_pos = 0,
      .expected_amt = 0,
      .expected = NULL,
    };
    free_tokens_res(tres);
    return res;
  }
  parse_tree_res pres = parse(tres.tokens, tres.token_amt);
  if (!pres.succeeded) {
    stringstream *ss = ss_init();
    token t = tres.tokens[pres.error_pos];
    format_error_ctx(ss->stream, input, t.start, t.end);
    char *error = ss_finalize_free(ss);
    failf(state, "Parsing failed:\n%s", error);
    free(error);
    free_parse_tree_res(pres);
  }
  free_tokens_res(tres);
  return pres;
}

tc_res test_upto_typecheck(test_state *state, const char *restrict input,
                           bool *success, parse_tree *tree) {
  parse_tree_res tree_res = test_upto_parse_tree(state, input);
  tc_res tc;
  if (tree_res.succeeded) {
    tc = typecheck(input, tree_res.tree);
    if (tc.error_amt > 0) {
      *success = false;
      stringstream *ss = ss_init();
      print_tc_errors(ss->stream, input, tree_res.tree, tc);
      char *error = ss_finalize_free(ss);
      failf(state, "Typecheck failed:\n%s", error);
      free(error);
      free_tc_res(tc);
      free_parse_tree_res(tree_res);
    } else {
      *success = true;
    }
    *tree = tree_res.tree;
  } else {
    *success = false;
    free_parse_tree_res(tree_res);
  }
  return tc;
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
  VEC_POP(&state->path);
  bool res = strstr(ss.string, state->filter_str) != NULL;
  free(ss.string);
  return res;
}
