#include <string.h>

#include "diagnostic.h"
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
  parse_tree_res pres = test_upto_parse_tree(state, input);
  if (pres.succeeded) {
    switch (output.tag) {
      case ANY:
        break;
      case STRING: {
        char *parse_tree_str = print_parse_tree_str(input, pres.tree);
        if (strcmp(parse_tree_str, output.str) != 0) {
          failf(state, "Different parse trees.\nExpected: '%s'\nGot: '%s'",
                output.str, parse_tree_str);
        }
        free(parse_tree_str);
        break;
      }
    }
  }
  free_parse_tree_res(pres);
}

static void test_parser_succeeds_on_form(test_state *state, char *input,
                                         expected_output output) {
  char *new_input;
  char *old_str = output.str;
  if (output.tag == STRING) {
    asprintf(&output.str, "(Fun (Lname a) () (Body %s))", old_str);
  }
  asprintf(&new_input, "(fun a () %s)", input);
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

  tokens_res tres = test_upto_tokens(state, input);
  if (!tres.succeeded)
    return;

  parse_tree_res pres = parse(tres.tokens, tres.token_amt);
  if (pres.succeeded) {
    char *parse_tree_str = print_parse_tree_str(input, pres.tree);
    failf(
      state,
      "Parsing \"%s\" was supposed to fail.\nGot parse tree:\n%s",
      input,
      parse_tree_str
    );
    free(pres.tree.inds);
    free(pres.tree.nodes);
    goto end_a;
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
    const char **a = alloca(sizeof(char *) * expected_amt);
    for (size_t i = 0; i < expected_amt; i++) {
      a[i] = yyTokenName[expected[i]];
    }
    char *as = join(expected_amt, a, ", ");
    const char **b = alloca(sizeof(char *) * pres.expected_amt);
    for (size_t i = 0; i < pres.expected_amt; i++) {
      b[i] = yyTokenName[pres.expected[i]];
    }
    char *bs = join(pres.expected_amt, b, ", ");
    failf(state, "Expected token mismatch.\nExpected: [%s]\nGot: [%s]", as, bs);
    free(as);
    free(bs);
  }

  free(pres.expected);
end_a:
  free(tres.tokens);
}

static void test_parser_fails_on_form(test_state *state, char *input,
                                      BUF_IND_T pos, NODE_IND_T expected_amt,
                                      const token_type *expected) {
  char *new_input;
  asprintf(&new_input, "(fun a () %s)", input);
  test_parser_fails_on(state, new_input, pos + 4, expected_amt, expected);
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
      fputs("(add (1, ", in->stream);
    }
    fputs("1", in->stream);
    for (size_t i = 0; i < depth; i++) {
      fputs("))", in->stream);
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
      expect_string("((Call (Lname hi) (Int 1)), (Int 2))"));
    test_end(state);
  }

  test_group_end(state);
}

static const token_type comma_or_expr_start[] = {
  TK_COMMA,      TK_INT,        TK_LOWER_NAME, TK_OPEN_BRACKET,
  TK_OPEN_PAREN, TK_UPPER_NAME, TK_STRING,     TK_UNIT};

#define expr_start (comma_or_expr_start + 1)
#define expr_start_amt (STATIC_LEN(comma_or_expr_start) - 1)

static void test_if_failures(test_state *state) {
  test_group_start(state, "If");

  {
    test_start(state, "Empty");
    test_parser_fails_on_form(state, "(if)", 2, expr_start_amt, expr_start);
    test_end(state);
  }

  {
    test_start(state, "One form");
    test_parser_fails_on_form(state, "(if 1)", 3, expr_start_amt, expr_start);
    test_end(state);
  }

  {
    test_start(state, "Two form");
    test_parser_fails_on_form(state, "(if 1 2)", 4, expr_start_amt, expr_start);
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
    test_parser_fails_on_form(state, "(if if 1 2 3)", 2, expr_start_amt,
                              expr_start);
    test_end(state);
  }

  test_group_end(state);
}

static token_type start_pattern[] = {TK_INT, TK_LOWER_NAME, TK_OPEN_BRACKET,
                                     TK_OPEN_PAREN, TK_UNIT};
static void test_fn_failures(test_state *state) {
  test_group_start(state, "Fn");

  {
    test_start(state, "Empty");
    test_parser_fails_on_form(state, "(fn)", 2, STATIC_LEN(start_pattern),
                              start_pattern);
    test_end(state);
  }

  {
    test_start(state, "Without body");
    test_parser_fails_on_form(state, "(fn ())", 3, expr_start_amt, expr_start);
    test_end(state);
  }

  test_group_end(state);
}

static void test_call_failures(test_state *state) {
  test_group_start(state, "Call");

  {
    test_start(state, "No params");
    test_parser_fails_on_form(state, "(a)", 2, STATIC_LEN(comma_or_expr_start),
                              comma_or_expr_start);
    test_end(state);
  }

  test_group_end(state);
}

static void test_fn_succeeds(test_state *state) {
  test_group_start(state, "Fn");

  {
    test_start(state, "Minimal");
    test_parser_succeeds_on_form(state, "(fn () 1)",
                                 expect_string("(Fn () (Body (Int 1)))"));
    test_end(state);
  }

  {
    test_start(state, "One arg");
    test_parser_succeeds_on_form(
      state, "(fn a 1)", expect_string("(Fn (Lname a) (Body (Int 1)))"));
    test_end(state);
  }

  {
    test_start(state, "Two args");
    test_parser_succeeds_on_form(
      state, "(fn (ab, cd) 1)",
      expect_string("(Fn ((Lname ab), (Lname cd)) (Body (Int 1)))"));
    test_end(state);
  }

  {
    test_start(state, "Numeric param");
    test_parser_succeeds_on_form(
      state, "(fn 123 1)", expect_string("(Fn (Int 123) (Body (Int 1)))"));
    test_end(state);
  }

  {
    test_start(state, "Nested unit param");
    test_parser_succeeds_on_form(state, "(fn (()) 1)",
                                 expect_string("(Fn () (Body (Int 1)))"));
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
    test_start(state, "Call");
    test_parser_succeeds_on_form(
      state, "(add (1, 2))",
      expect_string("(Call (Lname add) ((Int 1), (Int 2)))"));
    test_end(state);
  }

  {
    test_start(state, "Tuple");
    test_parser_succeeds_on_form(state, "(1, 2)",
                                 expect_string("((Int 1), (Int 2))"));
    test_end(state);
  }

  {
    test_start(state, "Typed");
    test_parser_succeeds_on_form(state, "(as I32 1)",
                                 expect_string("(As (Uname I32) (Int 1))"));
    test_end(state);
  }

  {
    test_start(state, "Big tuple");
    test_parser_succeeds_on_form(
      state, "(1, 2, 3, hi, 4)",
      expect_string("((Int 1), (Int 2), (Int 3), (Lname hi), (Int 4))"));
    test_end(state);
  }

  test_fn_succeeds(state);

  test_group_end(state);
}

static token_type inside_expr[] = {
  TK_AS,         TK_FN,         TK_FUN,          TK_IF,
  TK_INT,        TK_LOWER_NAME, TK_OPEN_BRACKET, TK_OPEN_PAREN,
  TK_UPPER_NAME, TK_STRING,     TK_UNIT};

static token_type inside_block_el[] = {
  TK_AS,         TK_FN,         TK_FUN,          TK_IF,
  TK_INT,        TK_LOWER_NAME, TK_OPEN_BRACKET, TK_OPEN_PAREN,
  TK_UPPER_NAME, TK_LET,        TK_STRING,       TK_UNIT};

static const size_t inside_expr_amt = STATIC_LEN(inside_expr);

static void test_mismatched_parens(test_state *state) {
  test_group_start(state, "Parens");

  {
    test_start(state, "Single open");
    test_parser_fails_on_form(state, "( ", 1, STATIC_LEN(inside_block_el),
                              inside_block_el);
    test_end(state);
  }

  {
    test_start(state, "Three open");
    test_parser_fails_on_form(state, "((( ", 3, inside_expr_amt, inside_expr);
    test_end(state);
  }

  {
    {
      test_start(state, "Single close");
      test_parser_fails_on_form(state, ")", 0, expr_start_amt, expr_start);
      test_end(state);
    }

    {
      test_start(state, "Three close");
      test_parser_fails_on_form(state, ")))", 0, expr_start_amt, expr_start);
      test_end(state);
    }
  }

  test_group_end(state);
}

static void test_parser_succeeds_on_type(test_state *state, char *in,
                                         char *ast) {
  char *full_ast;
  asprintf(&full_ast, "(Sig (Lname a) %s)", ast);
  char *full_in;
  asprintf(&full_in, "(sig a %s)", in);
  expected_output out = {.tag = STRING, .str = full_ast};
  test_parser_succeeds_on(state, full_in, out);
  free(full_ast);
  free(full_in);
}

static void test_parser_succeeds_type(test_state *state) {
  test_group_start(state, "Types");

  {
    char *in = "I32";
    test_start(state, in);
    test_parser_succeeds_on_type(state, in, "(Uname I32)");
    test_end(state);
  }

  {
    char *in = "(I32, I16)";
    test_start(state, in);
    test_parser_succeeds_on_type(state, in, "((Uname I32), (Uname I16))");
    test_end(state);
  }

  {
    char *in = "(I32, I16)";
    test_start(state, in);
    test_parser_succeeds_on_type(state, in, "((Uname I32), (Uname I16))");
    test_end(state);
  }

  test_group_end(state);
}

static void test_parser_succeeds_root(test_state *state) {
  {
    test_start(state, "Sig and fun");

    expected_output out = {.tag = STRING,
                           .str = "(Sig (Lname a) (Fn () (Uname I32)))\n"
                                  "(Fun (Lname a) () (Body (Int 12)))"};
    test_parser_succeeds_on(state,
                            "(sig a (Fn () I32))\n"
                            "(fun a () 12)",
                            out);

    test_end(state);
  }
  {
    test_start(state, "Multiple funs");

    expected_output out = {.tag = STRING,
                           .str = "(Fun (Lname a) () (Body (Lname a)))\n(Fun "
                                  "(Lname b) () (Body (Lname b)))"};
    test_parser_succeeds_on(state, "(fun a () a)(fun b () b)", out);

    test_end(state);
  }
}

static void test_parser_succeeds(test_state *state) {
  test_group_start(state, "Succeeds");

  test_parser_succeeds_atomic(state);
  test_parser_succeeds_compound(state);
  test_parser_succeeds_kitchen_sink(state);
  test_parser_succeeds_root(state);
  test_parser_succeeds_type(state);
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
  test_call_failures(state);

  test_group_end(state);
}

void test_parser(test_state *state) {
  test_group_start(state, "Parser");
  test_parser_succeeds(state);
  test_parser_fails(state);
  test_group_end(state);
}
