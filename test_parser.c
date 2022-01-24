#include <string.h>

#include "parse_tree.h"
#include "test.h"
#include "vec.h"

static void test_parser_succeeds(test_state *state, char *input, char *output) {

  source_file test_file = {.path = "parser-test", .data = input};
  tokens_res tres = scan_all(test_file);
  if (!tres.succeeded) {
    failf(state, "Parser test \"%s\" failed tokenization at position %d", input, tres.error_pos);
    goto ret;
  }

  parse_tree_res pres = parse(tres.tokens);
  if (!pres.succeeded) {
    failf(state, "Parsing \"%s\" failed.", input);
    goto ret;
  }

  stringstream ss;
  ss_init(&ss);
  print_parse_tree(ss.stream, test_file, pres.tree);
  ss_finalize(&ss);

  if (strcmp(ss.string, output) != 0) {
    failf(state, "Different parse trees.\nExpected: '%s'\nGot: '%s'", output, ss.string);
  }
  
ret:
  VEC_FREE(&tres.tokens);
  VEC_FREE(&pres.tree.inds);
  VEC_FREE(&pres.tree.nodes);
  free(ss.string);
}

static void test_parser_successes(test_state *state) {
  test_group_start(state, "Successes");

  {
    test_start(state, "Name");
    test_parser_succeeds(state, "hello", "(Name hello)");
    test_end(state);
  }

  {
    test_start(state, "Int");
    test_parser_succeeds(state, "123", "(Int 123)");
    test_end(state);
  }

  {
    test_start(state, "If");
    test_parser_succeeds(state, "(if 1 2 3)", "(If (Int 1) (Int 2) (Int 3))");
    test_end(state);
  }

  {
    test_start(state, "Call");
    test_parser_succeeds(state, "(add 1 2)", "(Call (Name add) (Int 1) (Int 2))");
    test_end(state);
  }

  test_group_end(state);
}

void test_parser(test_state *state) {
  test_group_start(state, "Parser");
  test_parser_successes(state);
  test_group_end(state);
}
