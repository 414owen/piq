#include "parse_tree.h"
#include "test.h"
#include "typecheck.h"

void test_typecheck_succeeds(test_state *state, char *input, char *output) {
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

void test_typecheck(test_state *state) {
  test_group_start(state, "Typecheck");

  // test_start(state, )

  test_group_end(state);
}
