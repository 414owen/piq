#include <string.h>

#include "parse_tree.h"
#include "test.h"
#include "vec.h"

static void test_parser_succeeds_on(test_state *state, char *input,
                                    char *output) {

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

  stringstream ss;
  ss_init(&ss);
  print_parse_tree(ss.stream, test_file, pres.tree);
  ss_finalize(&ss);
  if (strcmp(ss.string, output) != 0) {
    failf(state, "Different parse trees.\nExpected: '%s'\nGot: '%s'", output,
          ss.string);
  }
  free(ss.string);

end_b:
  VEC_FREE(&pres.tree.inds);
  VEC_FREE(&pres.tree.nodes);

end_a:
  VEC_FREE(&tres.tokens);
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

static void test_parser_succeeds_atomic(test_state *state) {
  test_group_start(state, "Atomic forms");
  {
    test_start(state, "Name");
    test_parser_succeeds_on(state, "hello", "(Lname hello)");
    test_end(state);
  }

  {
    test_start(state, "Int");
    test_parser_succeeds_on(state, "123", "(Int 123)");
    test_end(state);
  }
  test_group_end(state);
}

static void test_parser_succeeds_kitchen_sink(test_state *state) {
  test_group_start(state, "Kitchen sink");

  {
    test_start(state, "Lots of indentation");
    test_parser_succeeds_on(state, " ( ( hi 1   ) , 2 ) ",
                            "(Tup (Call (Lname hi) (Int 1)) (Int 2))");
    test_end(state);
  }

  test_group_end(state);
}

static const token_type form_start[] = {TK_INT, TK_UPPER_NAME, TK_LOWER_NAME,
                                        TK_OPEN_PAREN};

static void test_if_failures(test_state *state) {
  test_group_start(state, "If");

  {
    test_start(state, "Empty");
    test_parser_fails_on(state, "(if)", 2, STATIC_LEN(form_start), form_start);
    test_end(state);
  }

  {
    test_start(state, "One form");
    test_parser_fails_on(state, "(if 1)", 3, STATIC_LEN(form_start),
                         form_start);
    test_end(state);
  }

  {
    test_start(state, "Two form");
    test_parser_fails_on(state, "(if 1 2)", 4, STATIC_LEN(form_start),
                         form_start);
    test_end(state);
  }

  {
    test_start(state, "Four form");
    static token_type expected[] = {TK_CLOSE_PAREN};
    test_parser_fails_on(state, "(if 1 2 3 4)", 5, STATIC_LEN(expected),
                         expected);
    test_end(state);
  }

  {
    test_start(state, "Adjacent");
    test_parser_fails_on(state, "(if if 1 2 3)", 2, STATIC_LEN(form_start),
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
    test_parser_fails_on(state, "(fn)", 2, STATIC_LEN(expected), expected);
    test_end(state);
  }

  {
    test_start(state, "Without body");
    test_parser_fails_on(state, "(fn ())", 4, STATIC_LEN(form_start),
                         form_start);
    test_end(state);
  }

  test_group_start(state, "Params");
  {
    test_start(state, "Numeric param");
    static token_type expected[] = {TK_LOWER_NAME, TK_CLOSE_PAREN};
    test_parser_fails_on(state, "(fn (123: I32) 1)", 3, STATIC_LEN(expected),
                         expected);
    test_end(state);
  }

  {
    test_start(state, "Nest");
    static token_type expected[] = {TK_LOWER_NAME, TK_CLOSE_PAREN};
    test_parser_fails_on(state, "(fn (()))", 3, STATIC_LEN(expected), expected);
    test_end(state);
  }
  test_group_end(state);

  test_group_end(state);
}

static void test_fn_succeeds(test_state *state) {
  test_group_start(state, "Fn");

  {
    test_start(state, "Minimal");
    test_parser_succeeds_on(state, "(fn () 1)", "(Fn () (Int 1))");
    test_end(state);
  }

  {
    test_start(state, "One arg");
    test_parser_succeeds_on(state, "(fn (a: I32) 1)",
                            "(Fn ((Lname a): (Uname I32)) (Int 1))");
    test_end(state);
  }

  {
    test_start(state, "Two args");
    test_parser_succeeds_on(
      state, "(fn (ab: I16, cd: U64) 1)",
      "(Fn ((Lname ab): (Uname I16), (Lname cd): (Uname U64)) (Int 1))");
    test_end(state);
  }

  test_group_end(state);
}

static void test_parser_succeeds_compound(test_state *state) {
  test_group_start(state, "Compound forms");

  {
    test_start(state, "If");
    test_parser_succeeds_on(state, "(if 1 2 3)",
                            "(If (Int 1) (Int 2) (Int 3))");
    test_end(state);
  }

  {
    test_start(state, "Call without params");
    test_parser_succeeds_on(state, "(add)", "(Call (Lname add))");
    test_end(state);
  }

  {
    test_start(state, "Call");
    test_parser_succeeds_on(state, "(add 1 2)",
                            "(Call (Lname add) (Int 1) (Int 2))");
    test_end(state);
  }

  {
    test_start(state, "Tuple");
    test_parser_succeeds_on(state, "(1, 2)", "(Tup (Int 1) (Int 2))");
    test_end(state);
  }

  {
    test_start(state, "Typed");
    test_parser_succeeds_on(state, "(1 : I32)", "((Int 1): (Uname I32))");
    test_end(state);
  }

  {
    test_start(state, "Big tuple");
    test_parser_succeeds_on(state, "(1, 2, 3, hi, 4)",
                            "(Tup (Int 1) (Int 2) (Int 3) (Lname hi) (Int 4))");
    test_end(state);
  }

  test_fn_succeeds(state);

  test_group_end(state);
}

static void test_mismatched_parens(test_state *state) {
  test_group_start(state, "Parens");

  {
    test_start(state, "Single open");
    static token_type expected[] = {TK_INT,        TK_UPPER_NAME, TK_LOWER_NAME,
                                    TK_OPEN_PAREN, TK_FN,         TK_IF};
    test_parser_fails_on(state, "(", 1, STATIC_LEN(expected), expected);
    test_end(state);
  }

  {
    test_start(state, "Three open");
    static token_type expected[] = {TK_INT,        TK_UPPER_NAME, TK_LOWER_NAME,
                                    TK_OPEN_PAREN, TK_FN,         TK_IF};
    test_parser_fails_on(state, "(((", 3, STATIC_LEN(expected), expected);
    test_end(state);
  }

  {
    test_start(state, "Single close");
    static token_type expected[] = {TK_INT, TK_UPPER_NAME, TK_LOWER_NAME,
                                    TK_OPEN_PAREN};
    test_parser_fails_on(state, ")", 0, STATIC_LEN(expected), expected);
    test_end(state);
  }

  {
    test_start(state, "Three close");
    static token_type expected[] = {TK_INT, TK_UPPER_NAME, TK_LOWER_NAME,
                                    TK_OPEN_PAREN};
    test_parser_fails_on(state, ")))", 0, STATIC_LEN(expected), expected);
    test_end(state);
  }

  test_group_end(state);
}

static void test_parser_succeeds_root(test_state *state) {
  test_start(state, "Multiple top levels");

  test_parser_succeeds_on(state, "(1, 2) 3 4",
                          "(Tup (Int 1) (Int 2))\n(Int 3)\n(Int 4)");

  test_end(state);
}

static void test_parser_succeeds(test_state *state) {
  test_group_start(state, "Succeeds");

  test_parser_succeeds_atomic(state);
  test_parser_succeeds_compound(state);
  test_parser_succeeds_kitchen_sink(state);
  test_parser_succeeds_root(state);

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
