#include "consts.h"
#include "test.h"
#include "token.h"

static void test_scanner_tokens(test_state *state, char *buf, size_t token_amt,
                                const token_type *tokens) {
  BUF_IND_T ind = 0;
  source_file file = {.path = "TEST", .data = buf};
  for (int i = 0; i < token_amt; i++) {
    token_res t = scan(file, ind);
    test_assert_eq(state, t.succeeded, true);
    if (state->current_failed)
      break;
    test_assert_eq(state, t.token.type, tokens[i]);
    ind = t.token.end + 1;
  }
}

static void test_scanner_fails(test_state *state, char *buf,
                               BUF_IND_T err_pos) {
  BUF_IND_T ind = 0;
  source_file file = {.path = "TEST", .data = buf};
  bool seen_failure = true;
  for (;;) {
    token_res t = scan(file, ind);
    if (t.succeeded) {
      if (t.token.type == T_EOF)
        break;
      ind = t.token.end + 1;
      continue;
    }
    seen_failure = true;
    if (t.error_pos != err_pos)
      failf(state,
            "Wrong tokenizer error position.\n"
            "Expected: %" PRBI "\n"
            "Got: %" PRBI,
            t.error_pos, err_pos);
    return;
  }
  if (!seen_failure)
    failf(state, "Expected to see a tokenizer error for input: %s", buf);
}

static void test_scanner_accepts(test_state *state) {
  test_group_start(state, "Passes");

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
    static const token_type tokens[] = {T_OPEN_PAREN, T_OPEN_PAREN,
                                        T_CLOSE_PAREN, T_CLOSE_PAREN};
    test_scanner_tokens(state, "(())", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  {
    test_start(state, "Mismatching Parens");
    static const token_type tokens[] = {T_CLOSE_PAREN, T_CLOSE_PAREN,
                                        T_OPEN_PAREN, T_OPEN_PAREN};
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

  {
    test_start(state, "Kitchen Sink");
    static const token_type tokens[] = {T_NAME,       T_CLOSE_PAREN, T_NAME,
                                        T_OPEN_PAREN, T_NAME,        T_COMMA,
                                        T_INT,        T_NAME};
    test_scanner_tokens(state, "abc)b3(def,234a", STATIC_LEN(tokens), tokens);
    test_end(state);
  }

  test_group_end(state);
}

static void test_scanner_rejects(test_state *state) {
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
  test_group_end(state);
}
