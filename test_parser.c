#include <string.h>

#include "parse_tree.h"
#include "test.h"

static void test_parser_succeeds(test_state *state, char *input, char *output) {

  source_file test_file = {.path = "parser-test", .data = input};
  tokens_res tres = scan_all(test_file);
  if (!tres.succeeded) {
    failf(state, "Parser test \"%s\" failed tokenization at position %d", input, tres.error_pos);
    return;
  }

  parse_tree_res pres = parse(tres.tokens);

  stringstream ss;
  ss_init(&ss);
  print_parse_tree(ss.stream, pres.tree);
  ss_finalize(&ss);

  if (strcmp(ss.string, output) != 0) {
    failf(state, "Different parse trees.\nExpected: '%s'\nGot: '%s'", output, ss.string);
  }
}

static void test_parser_successes(test_state *state) {
  test_group_start(state, "Successes");

  {
    test_start(state, "Name");
    test_parser_succeeds(state, "hello", "(Name 'hello')");
    test_end(state);
  }

  test_group_end(state);
}

void test_parser(test_state *state) {
  test_group_start(state, "Parser");
  test_group_end(state);
}
