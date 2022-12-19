#include <stdbool.h>

#include "consts.h"
#include "defs.h"
#include "diagnostic.h"
#include "parser.h"
#include "test.h"
#include "tests.h"
#include "test_upto.h"
#include "token.h"

static source_file test_file(const char *restrict input) {
  source_file file = {.path = "TEST", .data = (char *)input};
  return file;
}

static void test_token_layout(test_state *restrict state) {
  test_group_start(state, "Token");
  test_start(state, "Is small");
  test_assert_eq(state, sizeof(token), 8);
  test_end(state);
  test_group_end(state);
}

static void test_scanner_tokens(test_state *restrict state,
                                char *restrict input, size_t token_amt,
                                const token_type *restrict tokens) {
  tokens_res tres = test_upto_tokens(state, input);

  add_scanner_timings(state, input, tres);

  if (tres.succeeded) {
    bool tokens_match = tres.token_amt == token_amt + 1;
    if (tokens_match) {
      for (size_t i = 0; i < token_amt; i++) {
        tokens_match &= tokens[i] == tres.tokens[i].type;
      }
    }
    if (!tokens_match) {
      char *exp = print_tokens_str(tokens, token_amt);
      size_t token_type_bytes = sizeof(token_type) * tres.token_amt;
      token_type *got_tokens = stalloc(token_type_bytes);
      for (size_t i = 0; i < tres.token_amt; i++) {
        got_tokens[i] = tres.tokens[i].type;
      }
      char *got = print_tokens_str(got_tokens, tres.token_amt);
      failf(state, "Token mismatch: Expected %s, got %s", exp, got);
      stfree(got_tokens, token_type_bytes);
      free(exp);
      free(got);
    }
    free(tres.tokens);
  } else {
    failf(state, "Expected scanner to succeed");
  }
}

static void test_scanner_fails(test_state *restrict state, char *restrict input,
                               buf_ind_t err_pos) {
  tokens_res tres = scan_all(test_file(input));

  add_scanner_timings(state, input, tres);

  if (tres.succeeded) {
    failf(state, "Expected to see a tokenizer error for input: %s", input);
    return;
  }
  if (tres.error_pos != err_pos) {
    failf(state,
          "Wrong tokenizer error position.\n"
          "Expected: %" PRBI "\n"
          "Got: %" PRBI,
          err_pos,
          tres.error_pos);
  }
}

static void test_scanner_accepts(test_state *restrict state) {
  test_group_start(state, "Passes");

  {
    test_start(state, "Empty");
    test_scanner_tokens(state, "", 0, NULL);
    test_end(state);
  }

  {
    test_start(state, "Open Paren");
    static const token_type tokens[] = {TK_OPEN_PAREN};
    test_scanner_tokens(state, "(", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Close Paren");
    static const token_type tokens[] = {TK_CLOSE_PAREN};
    test_scanner_tokens(state, ")", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Unit");
    static const token_type tokens[] = {TK_UNIT};
    test_scanner_tokens(state, "()", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Nested Parens");
    static const token_type tokens[] = {TK_OPEN_PAREN, TK_UNIT, TK_CLOSE_PAREN};
    test_scanner_tokens(state, "(())", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Mismatching Parens");
    static const token_type tokens[] = {
      TK_CLOSE_PAREN, TK_CLOSE_PAREN, TK_OPEN_PAREN, TK_OPEN_PAREN};
    test_scanner_tokens(state, "))((", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Name");
    static const token_type tokens[] = {TK_LOWER_NAME};
    test_scanner_tokens(state, "abc", STATIC_LEN(tokens), tokens);

    // These exercise some branches for code coverage, as well as testing close
    // shaves
    test_scanner_tokens(state, "fan", STATIC_LEN(tokens), tokens);
    test_scanner_tokens(state, "fud", STATIC_LEN(tokens), tokens);
    test_scanner_tokens(state, "funa", STATIC_LEN(tokens), tokens);
    test_scanner_tokens(state, "il", STATIC_LEN(tokens), tokens);
    test_scanner_tokens(state, "ifl", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Names");
    static const token_type tokens[] = {TK_LOWER_NAME, TK_LOWER_NAME};
    test_scanner_tokens(state, "abc def", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Fun");
    static const token_type tokens[] = {TK_FUN};
    test_scanner_tokens(state, "fun", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Sig");
    static const token_type tokens[] = {TK_SIG};
    test_scanner_tokens(state, "sig", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "If");
    static const token_type tokens[] = {TK_IF};
    test_scanner_tokens(state, "if", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Kitchen Sink");
    static const token_type tokens[] = {
      TK_LOWER_NAME,
      TK_CLOSE_PAREN,
      TK_LOWER_NAME,
      TK_OPEN_PAREN,
      TK_UPPER_NAME,
      TK_COMMA,
      TK_INT,
      TK_LOWER_NAME,
    };
    test_scanner_tokens(state, "abc)b3(Aef,234a", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "String");
    static const token_type tokens[] = {TK_STRING};
    test_scanner_tokens(state, "\"hi\"", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  test_group_end(state);
}

static void test_scan_all(test_state *restrict state) {
  test_group_start(state, "Scan all");

  {
    test_start(state, "Success");
    char *input = "(a)()";
    tokens_res res = scan_all(test_file(input));
    test_assert_eq(state, res.succeeded, true);
    test_assert_eq(state, res.token_amt, 5);
    free(res.tokens);
    test_end(state);
  }

  {
    test_start(state, "Failure");
    char *input = "();()";
    tokens_res res = scan_all(test_file(input));
    test_assert_eq(state, res.succeeded, false);
    test_assert_eq(state, res.error_pos, 2);
    free(res.tokens);
    test_end(state);
  }

  test_group_end(state);
}

static void test_scanner_rejects(test_state *restrict state) {
  test_group_start(state, "Rejects");

  {
    test_start(state, "all symbols");
    test_scanner_fails(state, ";;", 0);
    test_end(state);
  }

  {
    test_start(state, "start symbols");
    test_scanner_fails(state, ";;()", 0);
    test_end(state);
  }

  {
    test_start(state, "middle symbols");
    test_scanner_fails(state, "();;()", 2);
    test_end(state);
  }

  {
    test_start(state, "end symbols");
    test_scanner_fails(state, "();;", 2);
    test_end(state);
  }

  test_group_end(state);
}

void test_scanner(test_state *state) {
  test_group_start(state, "Scanner");
  test_token_layout(state);
  test_scanner_accepts(state);
  test_scanner_rejects(state);
  test_scan_all(state);
  test_group_end(state);
}
