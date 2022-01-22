#define _GNU_SOURCE

#include <stdarg.h>
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

static void fail_with(test_state *state, char *reason) {
  state->current_failed = true;
  failure f = { .path = make_test_path(state), .reason = reason };
  VEC_PUSH(&state->failures, f);
}

static void test_fail_eq(test_state *state, char *a, char *b) {
  fail_with(state, ne_reason(a, b));
}

#define test_assert_eq(state, a, b) \
  if ((a) != (b)) test_fail_eq(state, #a, #b);

static void test_start(test_state *state, char *name) {
  print_depth_indent(state);
  fputs(name, stdout);
  state->current_name = name;
  state->current_failed = false;
}

static void test_end(test_state *state) {
  printf(" %s" RESET "\n", state->current_failed ? RED "❌" : GRN "✓");
}

static void test_scanner_tokens(test_state *state, char *buf, size_t token_amt, const token_type *tokens) {
  BUF_IND_T ind = 0;
  source_file file = { .path = "TEST", .data = buf };
  for (int i = 0; i < token_amt; i++) {
    token_res t = scan(file, ind);
    test_assert_eq(state, t.succeeded, true);
    if (state->current_failed) break;
    test_assert_eq(state, t.token.type, tokens[i]);
    ind = t.token.end + 1;
  }
}

static void failf(test_state *state, const char *restrict fmt, ...) {
  char *err;
	va_list ap;
	va_start(ap, fmt);
	vasprintf(&err, fmt, ap);
  fail_with(state, err);
	va_end(ap);
}


static void test_scanner_fails(test_state *state, char *buf, char *err) {
  BUF_IND_T ind = 0;
  source_file file = { .path = "TEST", .data = buf };
  bool seen_failure = true;
  for (;;) {
    token_res t = scan(file, ind);
    if (t.succeeded) {
      if (t.token.type == T_EOF) break;
      ind = t.token.end + 1;
      continue;
    }
    seen_failure = true;
    if (t.error != err) failf(state, "Wrong tokenizer error. Expected:\n%s\bGot:\n%s", err, t.error);
    return;
  }
  if (!seen_failure) failf(state, "Expected to see a tokenizer error for input: %s", buf);
}

static void test_scanner(test_state *state) {
  test_group_start(state, "Scanner");

  {
    test_start(state, "Empty");
    static const token_type tokens[] = {};
    test_scanner_tokens(state, "", STATIC_LEN(tokens), tokens);
    test_end(state);
  }
  
  {
    test_start(state, "Open Paren");
    static const token_type tokens[] = {T_OPEN_PAREN};
    test_scanner_tokens(state, "(", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Close Paren");
    static const token_type tokens[] = {T_CLOSE_PAREN};
    test_scanner_tokens(state, ")", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Parens");
    static const token_type tokens[] = {T_OPEN_PAREN, T_CLOSE_PAREN};
    test_scanner_tokens(state, "()", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Nested Parens");
    static const token_type tokens[] = {T_OPEN_PAREN, T_OPEN_PAREN, T_CLOSE_PAREN, T_CLOSE_PAREN};
    test_scanner_tokens(state, "(())", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Mismatching Parens");
    static const token_type tokens[] = {T_CLOSE_PAREN, T_CLOSE_PAREN, T_OPEN_PAREN, T_OPEN_PAREN};
    test_scanner_tokens(state, "))((", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Name");
    static const token_type tokens[] = {T_NAME};
    test_scanner_tokens(state, "abc", STATIC_LEN(tokens), tokens);
    test_end(state);
  }
  
  {
    test_start(state, "Names");
    static const token_type tokens[] = {T_NAME, T_NAME};
    test_scanner_tokens(state, "abc def", STATIC_LEN(tokens), tokens);
    test_end(state);
  }
  
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
