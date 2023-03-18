#include <alloca.h>
#include <string.h>

#include "bitset.h"
#include "defs.h"
#include "diagnostic.h"
#include "parse_tree.h"
#include "test.h"
#include "test_upto.h"
#include "tests.h"
#include "util.h"
#include "vec.h"

// TODO use test_typecheck's span strategy

typedef struct {
  enum {
    ANY,
    STRING,
  } tag;
  char *str;
} expected_output;

static void test_parser_succeeds_on(test_state *state, const char *input,
                                    expected_output output) {
  parse_tree_res pres = test_upto_parse_tree(state, input);
  if (pres.type == PRT_SUCCESS) {
    switch (output.tag) {
      case ANY:
        break;
      case STRING: {
        char *parse_tree_str = print_parse_tree_str(input, pres.tree);
        if (strcmp(parse_tree_str, output.str) != 0) {
          failf(state,
                "Different parse trees.\n"
                "Expected: '%s'\n"
                "Got:      '%s'",
                output.str,
                parse_tree_str);
        }
        free(parse_tree_str);
        break;
      }
    }
  }
  free_parse_tree_res(pres);
}

static void test_parser_succeeds_on_form(test_state *state, char *input,
                                         const expected_output output) {
  expected_output new_exp = {
    .tag = output.tag,
  };
  if (output.tag == STRING) {
    new_exp.str = format_to_string("(Fun (TermName a) (Body %s))", output.str);
  }
  char *new_input = format_to_string("(fun a () %s)", input);
  test_parser_succeeds_on(state, new_input, output);
  free(new_input);
  if (output.tag == STRING) {
    free(new_exp.str);
  }
}

static void test_parser_fails_on(test_state *state, char *input, buf_ind_t pos,
                                 node_ind_t expected_amt,
                                 const token_type *expected) {

  tokens_res tres = test_upto_tokens(state, input);
  if (!tres.succeeded)
    return;

  parse_tree_res pres = parse(tres.tokens);

  add_parser_timings(state, tres, pres);

  switch (pres.type) {
    case PRT_SUCCESS: {
      char *parse_tree_str = print_parse_tree_str(input, pres.tree);
      failf(state,
            "Parsing \"%s\" was supposed to fail.\nGot parse tree:\n%s",
            input,
            parse_tree_str);
      free((void *)pres.tree.inds);
      free((void *)pres.tree.nodes);
      goto end_a;
    }
    case PRT_PARSE_ERROR:
      break;
  }

  if (pos != pres.error_pos) {
    failf(state,
          "Parsing failed at wrong position.\n"
          "Expected: %d\n"
          "Got:      %d",
          pos,
          pres.error_pos);
  }

  bool expected_tokens_match = multiset_eq(sizeof(token_type),
                                           pres.expected_amt,
                                           expected_amt,
                                           pres.expected,
                                           expected);

  if (!expected_tokens_match) {
    char *as = print_tokens_str(expected, expected_amt);
    char *bs = print_tokens_str(pres.expected, pres.expected_amt);
    failf(state,
          "Expected token mismatch.\n"
          "Expected: %s\n"
          "Got:      %s",
          as,
          bs);
    free(as);
    free(bs);
  }

  free(pres.expected);
end_a:
  free(tres.tokens);
}

static void test_parser_fails_on_form(test_state *state, char *input,
                                      buf_ind_t pos, node_ind_t expected_amt,
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
    test_parser_succeeds_on_form(
      state, "hello", expect_string("(TermName hello)"));
    test_end(state);
  }

  {
    test_start(state, "Int");
    test_parser_succeeds_on_form(state, "123", expect_string("(Int 123)"));
    test_end(state);
  }

  {
    test_start(state, "String");
    test_parser_succeeds_on_form(
      state, "\"hello\"", expect_string("\"hello\""));
    test_end(state);
  }

  test_group_end(state);
}

static void test_parser_robustness(test_state *state) {
  test_group_start(state, "Robustness");

  {
    test_start(state, "Nested calls");
    stringstream in;
    ss_init_immovable(&in);
    static const size_t depth = 100000;
    for (size_t i = 0; i < depth; i++) {
      fputs("(add (1, ", in.stream);
    }
    fputs("1", in.stream);
    for (size_t i = 0; i < depth; i++) {
      fputs("))", in.stream);
    }
    ss_finalize(&in);
    expected_output out = {.tag = ANY};
    test_parser_succeeds_on_form(state, in.string, out);
    free(in.string);
    test_end(state);
  }

  test_group_end(state);
}

static void test_parser_succeeds_kitchen_sink(test_state *state) {
  test_group_start(state, "Kitchen sink");

  {
    test_start(state, "Lots of whitespace");
    test_parser_succeeds_on_form(
      state,
      " ( ( hi 1   ) , 2 ) ",
      expect_string("((Call (TermName hi) (Int 1)), (Int 2))"));
    test_end(state);
  }

  test_group_end(state);
}

static const token_type comma_or_expression_start[] = {
  TKN_COMMA,
  TKN_INT,
  TKN_LOWER_NAME,
  TKN_OPEN_BRACKET,
  TKN_OPEN_PAREN,
  TKN_UPPER_NAME,
  TKN_STRING,
  TKN_UNIT,
};

#define expression_start (&comma_or_expression_start[1])
#define expression_start_amt (STATIC_LEN(comma_or_expression_start) - 1)

static void test_if_failures(test_state *state) {
  test_group_start(state, "If");

  {
    test_start(state, "Empty");
    test_parser_fails_on_form(
      state, "(if)", 2, expression_start_amt, expression_start);
    test_end(state);
  }

  {
    test_start(state, "One form");
    test_parser_fails_on_form(
      state, "(if 1)", 3, expression_start_amt, expression_start);
    test_end(state);
  }

  {
    test_start(state, "Two form");
    test_parser_fails_on_form(
      state, "(if 1 2)", 4, expression_start_amt, expression_start);
    test_end(state);
  }

  {
    test_start(state, "Four form");
    static token_type expected[] = {TKN_CLOSE_PAREN};
    test_parser_fails_on_form(
      state, "(if 1 2 3 4)", 5, STATIC_LEN(expected), expected);
    test_end(state);
  }

  {
    test_start(state, "Adjacent");
    test_parser_fails_on_form(
      state, "(if if 1 2 3)", 2, expression_start_amt, expression_start);
    test_end(state);
  }

  test_group_end(state);
}

static token_type start_pattern[] = {TKN_OPEN_PAREN,
                                     TKN_UNIT,
                                     TKN_INT,
                                     TKN_LOWER_NAME,
                                     TKN_OPEN_BRACKET,
                                     TKN_STRING};

static void test_fn_failures(test_state *state) {
  test_group_start(state, "Fn");

  {
    test_start(state, "Empty");
    test_parser_fails_on_form(state, "(fn)", 2, 2, start_pattern);
    test_end(state);
  }

  {
    test_start(state, "Without body");
    test_parser_fails_on_form(
      state, "(fn ())", 3, expression_start_amt, expression_start);
    test_end(state);
  }

  {
    test_start(state, "Nested unit param");
    static const token_type just_comma = TKN_COMMA;
    test_parser_fails_on_form(state, "(fn ((())) 1)", 5, 1, &just_comma);
    test_end(state);
  }

  test_group_end(state);
}

static void test_call_succeeds(test_state *state) {
  test_group_start(state, "Call");

  {
    test_start(state, "No params");
    test_parser_succeeds_on_form(
      state, "(a)", expect_string("(Call (TermName a))"));
    test_end(state);
  }

  test_group_end(state);
}

static void test_fn_succeeds(test_state *state) {
  test_group_start(state, "Fn");

  {
    test_start(state, "Minimal");
    test_parser_succeeds_on_form(
      state, "(fn () 1)", expect_string("(Fn () (Body (Int 1)))"));
    test_end(state);
  }

  {
    test_start(state, "One arg");
    test_parser_succeeds_on_form(
      state, "(fn (a) 1)", expect_string("(Fn ((Wildcard a)) (Body (Int 1)))"));
    test_end(state);
  }

  {
    test_start(state, "Two args");
    test_parser_succeeds_on_form(
      state,
      "(fn (ab cd) 1)",
      expect_string("(Fn ((Wildcard ab) (Wildcard cd)) (Body (Int 1)))"));
    test_end(state);
  }

  {
    test_start(state, "Numeric param");
    test_parser_succeeds_on_form(
      state, "(fn (123) 1)", expect_string("(Fn ((Int 123)) (Body (Int 1)))"));
    test_end(state);
  }

  test_group_end(state);
}

static void test_parser_succeeds_compound(test_state *state) {
  test_group_start(state, "Compound forms");

  {
    test_start(state, "List");
    test_parser_succeeds_on_form(
      state, "[1, 2, 3]", expect_string("[(Int 1), (Int 2), (Int 3)]"));
    test_end(state);
  }

  {
    test_start(state, "If");
    test_parser_succeeds_on_form(
      state, "(if 1 2 3)", expect_string("(If (Int 1) (Int 2) (Int 3))"));
    test_end(state);
  }

  {
    test_start(state, "Call");
    test_parser_succeeds_on_form(
      state,
      "(add (1, 2))",
      expect_string("(Call (TermName add) ((Int 1), (Int 2)))"));
    test_end(state);
  }

  {
    test_start(state, "Tuple");
    test_parser_succeeds_on_form(
      state, "(1, 2)", expect_string("((Int 1), (Int 2))"));
    test_end(state);
  }

  {
    test_start(state, "Typed");
    test_parser_succeeds_on_form(
      state,
      "(as I32 1)",
      expect_string("(As (TypeConstructorName I32) (Int 1))"));
    test_end(state);
  }

  {
    test_start(state, "Big tuple");
    test_parser_succeeds_on_form(
      state,
      "(1, 2, 3, hi, 4)",
      expect_string("((Int 1), (Int 2), (Int 3), (TermName hi), (Int 4))"));
    test_end(state);
  }

  test_fn_succeeds(state);

  test_group_end(state);
}

static token_type inside_expression[] = {
  TKN_AS,
  TKN_FN,
  TKN_FUN,
  TKN_IF,
  TKN_INT,
  TKN_LOWER_NAME,
  TKN_OPEN_BRACKET,
  TKN_OPEN_PAREN,
  TKN_UPPER_NAME,
  TKN_UNIT,
  TKN_STRING,
};

static token_type inside_block_el[] = {
  TKN_AS,
  TKN_FN,
  TKN_FUN,
  TKN_IF,
  TKN_INT,
  TKN_LOWER_NAME,
  TKN_OPEN_BRACKET,
  TKN_OPEN_PAREN,
  TKN_SIG,
  TKN_UPPER_NAME,
  TKN_LET,
  TKN_UNIT,
  TKN_STRING,
};

static const size_t inside_expression_amt = STATIC_LEN(inside_expression);

static void test_mismatched_parens(test_state *state) {
  test_group_start(state, "Parens");

  {
    test_start(state, "Single open");
    test_parser_fails_on_form(
      state, "( ", 1, STATIC_LEN(inside_block_el), inside_block_el);
    test_end(state);
  }

  {
    test_start(state, "Three open");
    test_parser_fails_on_form(
      state, "((( ", 3, inside_expression_amt, inside_expression);
    test_end(state);
  }

  {
    {
      test_start(state, "Single close");
      test_parser_fails_on_form(
        state, ")", 0, expression_start_amt, expression_start);
      test_end(state);
    }

    {
      test_start(state, "Three close");
      test_parser_fails_on_form(
        state, ")))", 0, expression_start_amt, expression_start);
      test_end(state);
    }
  }

  test_group_end(state);
}

static void test_parser_succeeds_on_type(test_state *state, char *in,
                                         char *ast) {
  char *full_ast;
  asprintf(&full_ast, "(Sig (TermName a) %s)", ast);
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
    test_parser_succeeds_on_type(state, in, "(TypeConstructorName I32)");
    test_end(state);
  }

  {
    char *in = "(I32, I16)";
    test_start(state, in);
    test_parser_succeeds_on_type(
      state, in, "((TypeConstructorName I32), (TypeConstructorName I16))");
    test_end(state);
  }

  {
    char *in = "(I32, I16)";
    test_start(state, in);
    test_parser_succeeds_on_type(
      state, in, "((TypeConstructorName I32), (TypeConstructorName I16))");
    test_end(state);
  }

  test_group_end(state);
}

static void test_parser_succeeds_root(test_state *state) {
  {
    test_start(state, "Sig and fun");

    expected_output out = {
      .tag = STRING,
      .str = "(Sig (TermName a) (Fn (()) (TypeConstructorName I32)))\n"
             "(Fun (TermName a) () (Body (Int 12)))"};
    test_parser_succeeds_on(state,
                            "(sig a (Fn () I32))\n"
                            "(fun a (()) 12)",
                            out);

    test_end(state);
  }
  {
    test_start(state, "Sig with params");

    expected_output out = {
      .tag = STRING,
      .str = "(Sig (TermName a) (Fn ((TypeConstructorName I8) "
             "(TypeConstructorName I16)) (TypeConstructorName I32)))"};
    test_parser_succeeds_on(state, "(sig a (Fn I8 I16 I32))", out);

    test_end(state);
  }
  {
    test_start(state, "Fun with params");

    expected_output out = {
      .tag = STRING,
      .str = "(Fun (TermName a) (Wildcard a) () (Body (Int 12)))"};
    test_parser_succeeds_on(state, "(fun a (a ()) 12)", out);

    test_end(state);
  }
  {
    test_start(state, "Multiple funs");

    expected_output out = {.tag = STRING,
                           .str = "(Fun (TermName a) (Body (TermName a)))\n"
                                  "(Fun (TermName b) (Body (TermName b)))"};
    test_parser_succeeds_on(state, "(fun a () a)(fun b () b)", out);

    test_end(state);
  }
}

static void test_parser_succeeds_statements(test_state *state) {
  test_group_start(state, "statements");
  {
    test_start(state, "Let");

    expected_output out = {
      .tag = STRING,
      .str = "(Fun (TermName a) (Body (Let (TermName b) ()) ()))",
    };
    test_parser_succeeds_on(state, "(fun a () (let b ()) ())", out);

    test_end(state);
  }
  test_group_end(state);
}

static void test_parser_succeeds_declaration(test_state *state) {
  test_group_start(state, "User-defined type");
  {
    test_start(state, "sum type");
    const char *input = "(data A () (Test))";
    expected_output out = {
      .tag = STRING,
      .str = "(DataDecl (TypeConstructorName A) (Type params: []) "
             "(DataConstructorName Test))",
    };
    test_parser_succeeds_on(state, input, out);

    test_end(state);
  }
  test_group_end(state);
}

static void test_parser_succeeds(test_state *state) {
  test_group_start(state, "Succeeds");

  test_parser_succeeds_atomic(state);
  test_parser_succeeds_declaration(state);
  test_parser_succeeds_compound(state);
  test_parser_succeeds_kitchen_sink(state);
  test_parser_succeeds_root(state);
  test_parser_succeeds_statements(state);
  test_parser_succeeds_type(state);
  if (!state->config.lite) {
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
  test_call_succeeds(state);
  test_parser_succeeds(state);
  test_parser_fails(state);
  test_group_end(state);
}
