#include <stdbool.h>
#include <stdio.h>

#include "diagnostic.h"
#include "llvm.h"
#include "parse_tree.h"
#include "test.h"
#include "typecheck.h"
#include "util.h"

static void test_typecheck_produces(test_state *state, char *input,
                                    size_t error_amt, uint32_t res) {
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

  tc_res tc_res = typecheck(test_file, pres.tree);
  if (tc_res.errors.len > 0) {
    failf(state, "Typechecking \"%s\" failed.", input);
    goto end_c;
  }

  LLVMModuleRef module = codegen(tc_res);

end_c:
  VEC_FREE(&tc_res.errors);
  VEC_FREE(&tc_res.types);
  VEC_FREE(&tc_res.type_inds);
  VEC_FREE(&tc_res.node_types);

end_b:
  VEC_FREE(&pres.tree.inds);
  VEC_FREE(&pres.tree.nodes);

end_a:
  VEC_FREE(&tres.tokens);
}

void test_typecheck(test_state *state) {
  test_group_start(state, "LLVM");

  test_group_end(state);
}
