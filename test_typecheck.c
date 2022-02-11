#include "parse_tree.h"
#include "test.h"
#include "typecheck.h"

static void test_typecheck_succeeds(test_state *state, char *input) {
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

  tc_res res = typecheck(test_file, pres.tree);
  if (res.errors.len > 0) {
    failf(state, "Typechecking failed for '%s", input);
  }

end_b:
  VEC_FREE(&pres.tree.inds);
  VEC_FREE(&pres.tree.nodes);

end_a:
  VEC_FREE(&tres.tokens);
}

void test_typecheck(test_state *state) {
  test_group_start(state, "Typecheck");

  test_start(state, "An I32");
  // test_typecheck_succeeds(state, "(fn a () I32 1)");
  test_end(state);

  test_group_end(state);
}
