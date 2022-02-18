#include <string.h>

#include "parse_tree.h"
#include "test.h"
#include "vec.h"

typedef struct {
  enum {
    ANY,
    STRING,
  } tag;
  char *str;
} expected_output;

static void test_parser_succeeds_on(test_state *state, char *input,
                                    expected_output output) {

  source_file test_file = {.path = "parser-test", .data = input};
  tokens_res tres = scan_all(test_file);
  if (!tres.succeeded) {
    failf(state, "Parser test \"%s\" failed tokenization at position %d", input,
          tres.error_pos);
    goto end_a;
  }

  parse_tree_res pres = parse(tres.tokens);
  if (!pres.succeeded) {
    failf(state, "Parsing \"%s\" failed.", input);
    goto end_b;
  }

  switch (output.tag) {
    case ANY:
      break;
    case STRING: {
      stringstream *ss = ss_init();
      print_parse_tree(ss->stream, test_file, pres.tree);
      char *str = ss_finalize(ss);
      if (strcmp(str, output.str) != 0) {
        failf(state, "Different parse trees.\nExpected: '%s'\nGot: '%s'",
              output.str, str);
      }
      free(str);
      break;
    }
  }

end_b:
  VEC_FREE(&pres.tree.inds);
  VEC_FREE(&pres.tree.nodes);

end_a:
  VEC_FREE(&tres.tokens);
}

static void test_parser_succeeds_on_form(test_state *state, char *input,
                                         expected_output output) {
  char *new_input;
  char *old_str = output.str;
  if (output.tag == STRING) {
    asprintf(&output.str, "(Let a (Fn () I32 %s))", old_str);
  }
  asprintf(&new_input, "(fn a () I32 %s)", input);
  test_parser_succeeds_on(state, new_input, output);
  free(new_input);
  if (output.tag == STRING) {
    free(output.str);
  }
}

extern char *yyTokenName[];

static void test_parser_fails_on(test_state *state, char *input, BUF_IND_T pos,
                                 NODE_IND_T expected_amt,
                                 const token_type *expected) {

  source_file test_file = {.path = "parser-test", .data = input};
  tokens_res tres = scan_all(test_file);
  if (!tres.succeeded) {
    failf(state, "Parser test \"%s\" failed tokenization at position %d", input,
          tres.error_pos);
    goto end_a;
  }

  parse_tree_res pres = parse(tres.tokens);
  if (pres.succeeded) {
    failf(state, "Parsing \"%s\" was supposed to fail.", input);
  }

  if (pos != pres.error_pos) {
    failf(state, "Parsing failed at wrong position.\nExpected: %d\nGot: %d",
          pos, pres.error_pos);
  }

  bool expected_tokens_match = pres.expected_amt == expected_amt;

  for (NODE_IND_T i = 0; expected_tokens_match && i < expected_amt; i++) {
    if (expected[i] != pres.expected[i])
      expected_tokens_match = false;
  }

  if (!expected_tokens_match) {
    char **a = alloca(sizeof(char *) * expected_amt);
    for (size_t i = 0; i < expected_amt; i++) {
      a[i] = yyTokenName[expected[i]];
    }
    char *as = join(expected_amt, a, ", ");
    char **b = alloca(sizeof(char *) * pres.expected_amt);
    for (size_t i = 0; i < pres.expected_amt; i++) {
      b[i] = yyTokenName[pres.expected[i]];
    }
    char *bs = join(pres.expected_amt, b, ", ");
    failf(state, "Expected token mismatch.\nExpected: [%s]\nGot: [%s]", as, bs);
    free(as);
    free(bs);
  }

  free(pres.expected);
  VEC_FREE(&pres.tree.inds);
  VEC_FREE(&pres.tree.nodes);

end_a:
  VEC_FREE(&tres.tokens);
}

static void test_parser_fails_on_form(test_state *state, char *input,
                                      BUF_IND_T pos, NODE_IND_T expected_amt,
                                      const token_type *expected) {
  char *new_input;
  asprintf(&new_input, "(fn a () I32 %s)", input);
  test_parser_fails_on(state, new_input, pos + 6, expected_amt, expected);
  free(new_input);
}

expected_output expect_string(char *str) {
  expected_output res = {
    .tag = STRING,
    .str = str,
  };
  return res;
}

static void test_parser_succeeds_atomic(test_state *state) {
  test_group_start(state, "Atomic forms");

  {
    test_start(state, "Name");
    test_parser_succeeds_on_form(state, "hello",
                                 expect_string("(Lname hello)"));
    test_end(state);
  }

  {
    test_start(state, "Int");
    test_parser_succeeds_on_form(state, "123", expect_string("(Int 123)"));
    test_end(state);
  }

  {
    test_start(state, "String");
    test_parser_succeeds_on_form(state, "\"hello\"",
                                 expect_string("\"hello\""));
    test_end(state);
  }

  test_group_end(state);
}

static void test_parser_robustness(test_state *state) {
  test_group_start(state, "Robustness");

  {
    test_start(state, "Nested calls");
    stringstream *in = ss_init();
    static const size_t depth = 1000000;
    for (size_t i = 0; i < depth; i++) {
      fputs("(add 1 ", in->stream);
    }
    fputs("1", in->stream);
    for (size_t i = 0; i < depth; i++) {
      putc(')', in->stream);
    }
    char *str = ss_finalize(in);
    expected_output out = {.tag = ANY};
    test_parser_succeeds_on_form(state, str, out);
    test_end(state);
    free(str);
  }

  test_group_end(state);
}

static void test_parser_succeeds_kitchen_sink(test_state *state) {
  test_group_start(state, "Kitchen sink");

  {
    test_start(state, "Lots of indentation");
    test_parser_succeeds_on_form(
      state, " ( ( hi 1   ) , 2 ) ",
      expect_string("(Tup (Call (Lname hi) (Int 1)) (Int 2))"));
    test_end(state);
  }

  test_group_end(state);
}

static const token_type form_start[] = {TK_INT,        TK_UPPER_NAME,
                                        TK_LOWER_NAME, TK_OPEN_PAREN,
                                        TK_STRING,     TK_OPEN_BRACKET};

static void test_if_failures(test_state *state) {
  test_group_start(state, "If");

  {
    test_start(state, "Empty");
    test_parser_fails_on_form(state, "(if)", 2, STATIC_LEN(form_start),
                              form_start);
    test_end(state);
  }

  {
    test_start(state, "One form");
    test_parser_fails_on_form(state, "(if 1)", 3, STATIC_LEN(form_start),
                              form_start);
    test_end(state);
  }

  {
    test_start(state, "Two form");
    test_parser_fails_on_form(state, "(if 1 2)", 4, STATIC_LEN(form_start),
                              form_start);
    test_end(state);
  }

  {
    test_start(state, "Four form");
    static token_type expected[] = {TK_CLOSE_PAREN};
    test_parser_fails_on_form(state, "(if 1 2 3 4)", 5, STATIC_LEN(expected),
                              expected);
    test_end(state);
  }

  {
    test_start(state, "Adjacent");
    test_parser_fails_on_form(state, "(if if 1 2 3)", 2, STATIC_LEN(form_start),
                              form_start);
    test_end(state);
  }

  test_group_end(state);
}

static void test_fn_failures(test_state *state) {
  test_group_start(state, "Fn");

  {
    test_start(state, "Empty");
    static token_type expected[] = {TK_OPEN_PAREN};
    test_parser_fails_on_form(state, "(fn)", 2, STATIC_LEN(expected), expected);
    test_end(state);
  }

  {
    test_start(state, "Without body");
    static token_type expected[] = {TK_UPPER_NAME};
    test_parser_fails_on_form(state, "(fn ())", 4, STATIC_LEN(expected),
                              expected);
    test_end(state);
  }

  test_group_start(state, "Params");
  {
    test_start(state, "Numeric param");
    static token_type expected[] = {TK_LOWER_NAME, TK_CLOSE_PAREN};
    test_parser_fails_on_form(state, "(fn (123: I32) 1)", 3,
                              STATIC_LEN(expected), expected);
    test_end(state);
  }

  {
    test_start(state, "Nest");
    static token_type expected[] = {TK_LOWER_NAME, TK_CLOSE_PAREN};
    test_parser_fails_on_form(state, "(fn (()))", 3, STATIC_LEN(expected),
                              expected);
    test_end(state);
  }
  test_group_end(state);

  test_group_end(state);
}

static void test_fn_succeeds(test_state *state) {
  test_group_start(state, "Fn");

  {
    test_start(state, "Minimal");
    test_parser_succeeds_on_form(state, "(fn () I32 1)",
                                 expect_string("(Fn () I32 (Int 1))"));
    test_end(state);
  }

  {
    test_start(state, "One arg");
    test_parser_succeeds_on_form(
      state, "(fn (a: I32) I16 1)",
      expect_string("(Fn ((Lname a): (Uname I32)) I16 (Int 1))"));
    test_end(state);
  }

  {
    test_start(state, "Two args");
    test_parser_succeeds_on_form(
      state, "(fn (ab: I16, cd: U64) I32 1)",
      expect_string(
        "(Fn ((Lname ab): (Uname I16), (Lname cd): (Uname U64)) I32 (Int 1))"));
    test_end(state);
  }

  test_group_end(state);
}

static void test_parser_succeeds_compound(test_state *state) {
  test_group_start(state, "Compound forms");

  {
    test_start(state, "List");
    test_parser_succeeds_on_form(state, "[1, 2, 3]",
                                 expect_string("[(Int 1), (Int 2), (Int 3)]"));
    test_end(state);
  }

  {
    test_start(state, "If");
    test_parser_succeeds_on_form(state, "(if 1 2 3)",
                                 expect_string("(If (Int 1) (Int 2) (Int 3))"));
    test_end(state);
  }

  {
    test_start(state, "Call without params");
    test_parser_succeeds_on_form(state, "(add)",
                                 expect_string("(Call (Lname add))"));
    test_end(state);
  }

  {
    test_start(state, "Call");
    test_parser_succeeds_on_form(
      state, "(add 1 2)", expect_string("(Call (Lname add) (Int 1) (Int 2))"));
    test_end(state);
  }

  {
    test_start(state, "Tuple");
    test_parser_succeeds_on_form(state, "(1, 2)",
                                 expect_string("(Tup (Int 1) (Int 2))"));
    test_end(state);
  }

  {
    test_start(state, "Typed");
    test_parser_succeeds_on_form(state, "(1 : I32)",
                                 expect_string("((Int 1): (Uname I32))"));
    test_end(state);
  }

  {
    test_start(state, "Big tuple");
    test_parser_succeeds_on_form(
      state, "(1, 2, 3, hi, 4)",
      expect_string("(Tup (Int 1) (Int 2) (Int 3) (Lname hi) (Int 4))"));
    test_end(state);
  }

  test_fn_succeeds(state);

  test_group_end(state);
}

static void test_mismatched_parens(test_state *state) {
  test_group_start(state, "Parens");

  {
    test_start(state, "Single open");
    static token_type expected[] = {
      TK_INT, TK_UPPER_NAME, TK_LOWER_NAME, TK_OPEN_PAREN,
      TK_FN,  TK_IF,         TK_STRING,     TK_OPEN_BRACKET};
    test_parser_fails_on_form(state, "(", 1, STATIC_LEN(expected), expected);
    test_end(state);
  }

  {
    test_start(state, "Three open");
    static token_type expected[] = {
      TK_INT, TK_UPPER_NAME, TK_LOWER_NAME, TK_OPEN_PAREN,
      TK_FN,  TK_IF,         TK_STRING,     TK_OPEN_BRACKET};
    test_parser_fails_on_form(state, "(((", 3, STATIC_LEN(expected), expected);
    test_end(state);
  }

  {
    test_start(state, "Single close");
    static token_type expected[] = {TK_INT,        TK_UPPER_NAME,
                                    TK_LOWER_NAME, TK_OPEN_PAREN,
                                    TK_STRING,     TK_OPEN_BRACKET};
    test_parser_fails_on_form(state, ")", 0, STATIC_LEN(expected), expected);
    test_end(state);
  }

  {
    test_start(state, "Three close");
    static token_type expected[] = {TK_INT,        TK_UPPER_NAME,
                                    TK_LOWER_NAME, TK_OPEN_PAREN,
                                    TK_STRING,     TK_OPEN_BRACKET};
    test_parser_fails_on_form(state, ")))", 0, STATIC_LEN(expected), expected);
    test_end(state);
  }

  test_group_end(state);
}

static void test_parser_succeeds_root(test_state *state) {
  test_start(state, "Multiple top levels");

  expected_output out = {
    .tag = STRING,
    .str = "(Let a (Fn () I16 (Lname a)))\n(Let b (Fn () I32 (Lname b)))"};
  test_parser_succeeds_on(state, "(fn a () I16 a)(fn b () I32 b)", out);

  test_end(state);
}

static void test_parser_succeeds(test_state *state) {
  test_group_start(state, "Succeeds");

  test_parser_succeeds_atomic(state);
  test_parser_succeeds_compound(state);
  test_parser_succeeds_kitchen_sink(state);
  test_parser_succeeds_root(state);
  if (!state->lite) {
    test_parser_robustness(state);
  }

  test_group_end(state);
}

static void test_parser_fails(test_state *state) {
  test_group_start(state, "Fails");

  test_mismatched_parens(state);
  test_if_failures(state);
  test_fn_failures(state);

  test_group_end(state);
}

void test_parser(test_state *state) {
  test_group_start(state, "Parser");
  test_parser_succeeds(state);
  test_parser_fails(state);
  test_group_end(state);
}
