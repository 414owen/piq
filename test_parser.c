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
    VEC_FREE(&tres.tokens);
    return;
  }

  parse_tree_res pres = parse(tres.tokens);
  if (!pres.succeeded) {
    failf(state, "Parsing \"%s\" failed.", input);
    VEC_FREE(&pres.tree.inds);
    VEC_FREE(&pres.tree.nodes);
    return;
  }

  stringstream ss;
  ss_init(&ss);
  print_parse_tree(ss.stream, test_file, pres.tree);
  ss_finalize(&ss);

  if (strcmp(ss.string, output) != 0) {
    failf(state, "Different parse trees.\nExpected: '%s'\nGot: '%s'", output,
          ss.string);
    VEC_FREE(&pres.tree.inds);
    VEC_FREE(&pres.tree.nodes);
    free(ss.string);
  }
}

static void test_parser_fails_on(test_state *state, char *input,
                                 BUF_IND_T pos) {

  source_file test_file = {.path = "parser-test", .data = input};
  tokens_res tres = scan_all(test_file);
  if (!tres.succeeded) {
    failf(state, "Parser test \"%s\" failed tokenization at position %d", input,
          tres.error_pos);
    goto ret;
  }

  parse_tree_res pres = parse(tres.tokens);
  if (pres.succeeded) {
    failf(state, "Parsing \"%s\" was supposed to fail.", input);
    goto ret;
  }

  if (pos != pres.error_pos) {
    failf(state, "Parsing failed at wrong position.\nExpected: %d\nGot: %d",
          pos, pres.error_pos);
    goto ret;
  }

ret:
  VEC_FREE(&tres.tokens);
}

static void test_parser_succeeds_atomic(test_state *state) {
  {
    test_start(state, "Name");
    test_parser_succeeds_on(state, "hello", "(Name hello)");
    test_end(state);
  }

  {
    test_start(state, "Int");
    test_parser_succeeds_on(state, "123", "(Int 123)");
    test_end(state);
  }
}

static void test_parser_succeeds_compound(test_state *state) {
  {
    test_start(state, "If");
    test_parser_succeeds_on(state, "(if 1 2 3)",
                            "(If (Int 1) (Int 2) (Int 3))");
    test_end(state);
  }

  {
    test_start(state, "Call without params");
    test_parser_succeeds_on(state, "(add)", "(Call (Name add))");
    test_end(state);
  }

  {
    test_start(state, "Call");
    test_parser_succeeds_on(state, "(add 1 2)",
                            "(Call (Name add) (Int 1) (Int 2))");
    test_end(state);
  }

  {
    test_start(state, "Tuple");
    test_parser_succeeds_on(state, "(1, 2)", "(Tup (Int 1) (Int 2))");
    test_end(state);
  }

  {
    test_start(state, "Big tuple");
    test_parser_succeeds_on(state, "(1, 2, 3, hi, 4)",
                            "(Tup (Int 1) (Int 2) (Int 3) (Name hi) (Int 4))");
    test_end(state);
  }
}

static void test_parser_succeeds_kitchen_sink(test_state *state) {
  {
    test_start(state, "Lots of indentation");
    test_parser_succeeds_on(state, " ( ( hi 1   ) , 2 ) ",
                            "(Tup (Call (Name hi) (Int 1)) (Int 2))");
    test_end(state);
  }
}

static void test_parser_succeeds(test_state *state) {
  test_group_start(state, "Succeeds");

  test_group_start(state, "Atomic forms");
  test_parser_succeeds_atomic(state);
  test_group_end(state);

  test_group_start(state, "Compound forms");
  test_parser_succeeds_compound(state);
  test_group_end(state);

  test_group_start(state, "Kitchen sink");
  test_parser_succeeds_kitchen_sink(state);
  test_group_end(state);

  test_group_end(state);
}

static void test_parser_fails(test_state *state) {
  test_group_start(state, "Fails");

  {
    test_start(state, "too many open parens");
    test_parser_fails_on(state, "(", 0);
    test_end(state);
  }

  {
    test_start(state, "too many close parens");
    test_parser_fails_on(state, ")", 0);
    test_end(state);
  }

  test_group_end(state);
}

void test_parser(test_state *state) {
  test_group_start(state, "Parser");
  test_parser_succeeds(state);
  test_parser_fails(state);
  test_group_end(state);
}
