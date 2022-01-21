#include <stdio.h>
#include <stdbool.h>

#include "term.h"
#include "token.h"
#include "util.h"
#include "vec.h"

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

static const int test_indent = 2;

static void print_depth_indent(test_state *state) {
  for (int i = 0; i < state->path.len * test_indent; i++) {
    putc(' ', stdout);
  }
}

static void test_group_start(test_state *state, char *name) {
  print_depth_indent(state);
  printf("%s\n", name);
  VEC_PUSH(&state->path, name);
}

static void test_group_end(test_state *state) {
  VEC_POP(&state->path);
}

static char *test_parens = "()";
static token_type test_paren_tokens[] = {T_OPEN_PAREN, T_CLOSE_PAREN};

static vec_string make_test_path(test_state *state) {
  vec_string path = VEC_CLONE(&state->path);
  VEC_PUSH(&path, state->current_name);
  return path;
}

static char *ne_reason(char *a_name, char *b_name) {
  char *strs[] = {"Assert failed: ", a_name, " != ", b_name};
  char *res = join(STATIC_LEN(strs), strs);
  return res;
}

#define test_assert_eq(state, a, b) \
  if ((a) != (b)) { \
    state->current_failed = true; \
    failure f = { .path = make_test_path(state), .reason = ne_reason(#a, #b) }; \
    VEC_PUSH(&state->failures, f); \
  }

static void test_start(test_state *state, char *name) {
  print_depth_indent(state);
  fputs(name, stdout);
  state->current_name = name;
  state->current_failed = false;
}

static void test_end(test_state *state) {
  printf(" %s" RESET "\n", state->current_failed ? RED "❌" : GRN "✓");
}

static void test_scanner_parens(test_state *state) {
  test_start(state, "Parens");
  BUF_IND_T ind = 0;
  for (int i = 0; i < STATIC_LEN(test_paren_tokens); i++) {
    token t = scan(test_parens, ind);
    test_assert_eq(state, t.type, test_paren_tokens[i]);
    ind = t.end + 1;
  }
  test_end(state);
}

static void test_scanner(test_state *state) {
  test_group_start(state, "Scanner");
  test_scanner_parens(state);
  test_group_end(state);
}

static test_state test_state_new(void) {
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
    if (i != 0) putc('/', stdout);
    fputs(f.path.data[i], stdout);
  }
  putc('\n', stdout);
  puts(f.reason);
}

static void print_failures(test_state *state) {
  for (size_t i = 0; i < state->failures.len; i++) {
    print_failure(state->failures.data[i]);
  }
}

int main(int argc, char **argv) {
  test_state state = test_state_new();
  test_scanner(&state);
  print_failures(&state);
}
