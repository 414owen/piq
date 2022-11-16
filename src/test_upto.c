#include "diagnostic.h"
#include "test.h"
#include "test_upto.h"

parse_tree_res test_upto_parse_tree(test_state *state,
                                    const char *restrict input) {
  tokens_res tres = test_upto_tokens(state, input);
  if (!tres.succeeded) {
    parse_tree_res res = {
      .succeeded = false,
      .error_pos = 0,
      .expected_amt = 0,
      .expected = NULL,
    };
    free_tokens_res(tres);
    return res;
  }
  parse_tree_res pres = parse(tres.tokens, tres.token_amt);
  if (!pres.succeeded) {
    char *error = print_parse_tree_error_string(input, tres.tokens, pres);
    failf(state, "Parsing failed:\n%s", error);
    free(error);
  }
  free_tokens_res(tres);
  return pres;
}

tc_res test_upto_typecheck(test_state *state, const char *restrict input,
                           bool *success, parse_tree *tree) {
  parse_tree_res tree_res = test_upto_parse_tree(state, input);
  tc_res tc;
  if (tree_res.succeeded) {
    tc = typecheck(input, tree_res.tree);
    if (tc.error_amt > 0) {
      *success = false;
      stringstream *ss = ss_init();
      print_tc_errors(ss->stream, input, tree_res.tree, tc);
      char *error = ss_finalize_free(ss);
      failf(state, "Typecheck failed:\n%s", error);
      free(error);
      free_tc_res(tc);
      free_parse_tree_res(tree_res);
    } else {
      *success = true;
    }
    *tree = tree_res.tree;
  } else {
    *success = false;
    free_parse_tree_res(tree_res);
  }
  return tc;
}

