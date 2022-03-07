#include <stdbool.h>

#include "consts.h"
#include "test.h"
#include "token.h"

static source_file test_file(const char *restrict input) {
  source_file file = {.path = "TEST", .data = (char *)input};
  return file;
}

static void test_scanner_tokens(test_state *restrict state, char *restrict buf,
                                size_t token_amt,
                                const token_type *restrict tokens) {
  BUF_IND_T ind = 0;
  for (size_t i = 0; i < token_amt; i++) {
    token_res t = scan(test_file(buf), ind);
    test_assert_eq(state, t.succeeded, true);
    if (state->current_failed)
      break;
    test_assert_eq(state, t.tok.type, tokens[i]);
    ind = t.tok.end + 1;
  }
}

static void test_scanner_fails(test_state *restrict state, char *restrict buf,
                               BUF_IND_T err_pos) {
  BUF_IND_T ind = 0;
  bool seen_failure = true;
  for (;;) {
    token_res t = scan(test_file(buf), ind);
    if (t.succeeded) {
      if (t.tok.type == TK_EOF)
        break;
      ind = t.tok.end + 1;
      continue;
    }
    seen_failure = true;
    if (t.tok.start != err_pos)
      failf(state,
            "Wrong tokenizer error position.\n"
            "Expected: %" PRBI "\n"
            "Got: %" PRBI,
            t.tok.start, err_pos);
    return;
  }
  if (!seen_failure)
    failf(state, "Expected to see a tokenizer error for input: %s", buf);
}

static void test_scanner_accepts(test_state *restrict state) {
  test_group_start(state, "Passes");

  {
    test_start(state, "Empty");
    static const token_type tokens[] = {};
    test_scanner_tokens(state, "", STATIC_LEN(tokens), tokens);
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
    test_start(state, "Parens");
    static const token_type tokens[] = {TK_OPEN_PAREN, TK_CLOSE_PAREN};
    test_scanner_tokens(state, "()", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Nested Parens");
    static const token_type tokens[] = {TK_OPEN_PAREN, TK_OPEN_PAREN,
                                        TK_CLOSE_PAREN, TK_CLOSE_PAREN};
    test_scanner_tokens(state, "(())", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Mismatching Parens");
    static const token_type tokens[] = {TK_CLOSE_PAREN, TK_CLOSE_PAREN,
                                        TK_OPEN_PAREN, TK_OPEN_PAREN};
    test_scanner_tokens(state, "))((", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Name");
    static const token_type tokens[] = {TK_LOWER_NAME};
    test_scanner_tokens(state, "abc", STATIC_LEN(tokens), tokens);

    // These exercise some branches for code coverage, as well as testing close
    // shaves
    test_scanner_tokens(state, "fa", STATIC_LEN(tokens), tokens);
    test_scanner_tokens(state, "fna", STATIC_LEN(tokens), tokens);
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
    test_start(state, "If");
    static const token_type tokens[] = {TK_IF};
    test_scanner_tokens(state, "if", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Kitchen Sink");
    static const token_type tokens[] = {
      TK_LOWER_NAME, TK_CLOSE_PAREN, TK_LOWER_NAME, TK_OPEN_PAREN,
      TK_UPPER_NAME, TK_COMMA,       TK_INT,        TK_LOWER_NAME};
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
    test_assert_eq(state, res.tokens.len, 6);
    VEC_FREE(&res.tokens);
    test_end(state);
  }

  {
    test_start(state, "Failure");
    char *input = "();()";
    tokens_res res = scan_all(test_file(input));
    test_assert_eq(state, res.succeeded, false);
    test_assert_eq(state, res.error_pos, 2);
    VEC_FREE(&res.tokens);
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
  test_scanner_accepts(state);
  test_scanner_rejects(state);
  test_scan_all(state);
  test_group_end(state);
}
