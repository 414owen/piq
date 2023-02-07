#include "defs.h"
#include "diagnostic.h"
#include "test.h"
#include "test_upto.h"

#ifdef TIME_TOKENIZER
// TODO use pre-known file sizes for non-tests
void add_scanner_timings_internal(test_state *state, const char *restrict input,
                                  tokens_res tres) {
  if (tres.succeeded) {
    state->total_bytes_tokenized += strlen(input);
  } else {
    state->total_bytes_tokenized += tres.error_pos;
  }
  state->total_tokenization_time =
    timespec_add(state->total_tokenization_time, tres.time_taken);
  state->total_tokens += tres.token_amt;
}
#endif

#ifdef TIME_PARSER
void add_parser_timings_internal(test_state *state, tokens_res tres,
                                 parse_tree_res pres) {
  switch (pres.type) {
    case PRT_SEMANTIC_ERRORS:
      state->total_tokens_parsed += pres.error_pos;
      break;
    case PRT_SUCCESS:
    case PRT_PARSE_ERROR:
      state->total_tokens_parsed += tres.token_amt;
      break;
  }
  state->total_parser_time =
    timespec_add(state->total_parser_time, pres.time_taken);
  state->total_parse_nodes_produced += pres.tree.node_amt;
}
#endif

#ifdef TIME_TYPECHECK
void add_typecheck_timings_internal(test_state *state, parse_tree tree,
                                    tc_res tc_res) {
  if (tc_res.error_amt == 0) {
    state->total_typecheck_time =
      timespec_add(state->total_typecheck_time, tc_res.time_taken);
    state->total_parse_nodes_typechecked += tree.node_amt;
  }
}
#endif

#ifdef TIME_CODEGEN
void add_codegen_timings_internal(test_state *state, parse_tree tree,
                                  llvm_res llres) {
  state->total_llvm_ir_generation_time =
    timespec_add(state->total_llvm_ir_generation_time, llres.time_taken);
  state->total_parse_nodes_codegened += tree.node_amt;
}
#endif

/*
#ifdef TIME_TYPECHECK
void add_typecheck_timings_internal(test_state *state, tokens_res tres,
parse_tree_res pres) { if (pres.succeeded) { state->total_tokens_parsed +=
tres.token_amt; } else { state->total_tokens_parsed += pres.error_pos;
  }
  state->total_parser_time = timespec_add(state->total_parser_time,
pres.time_taken); state->total_parse_nodes_produced += pres.tree.node_amt;
}
#endif
*/

tokens_res test_upto_tokens(test_state *state, const char *restrict input) {
  source_file test_file = {.path = "parser-test", .data = input};
  tokens_res tres = scan_all(test_file);

  add_scanner_timings(state, input, tres);

  if (!tres.succeeded) {
    stringstream ss;
    ss_init_immovable(&ss);
    format_error_ctx(ss.stream, input, tres.error_pos, 1);
    ss_finalize(&ss);
    failf(state, "Scanning failed:\n%s", ss.string);
    free_tokens_res(tres);
    free(ss.string);
  }

  return tres;
}

parse_tree_res test_upto_parse_tree(test_state *state,
                                    const char *restrict input) {
  tokens_res tres = test_upto_tokens(state, input);
  add_scanner_timings(state, input, tres);
  if (!tres.succeeded) {
    parse_tree_res res = {
      .type = PRT_SEMANTIC_ERRORS,
      .error_pos = 0,
      .expected_amt = 0,
      .expected = NULL,
    };
    failf(state, "Tokenization failed at: %u", tres.error_pos);
    free_tokens_res(tres);
    return res;
  }

  const parse_tree_res pres = parse(tres.tokens, tres.token_amt);

  add_parser_timings(state, tres, pres);

  if (pres.type != PRT_SUCCESS) {
    char *error = print_parse_errors_string(input, tres.tokens, pres);
    failf(state, "Parsing failed:\n%s", error);
    free(error);
  }
  free_tokens_res(tres);
  return pres;
}

upto_resolution_res test_upto_resolution(test_state *state,
                                         const char *restrict input) {
  parse_tree_res tree_res = test_upto_parse_tree(state, input);
  if (tree_res.type != PRT_SUCCESS) {
    free_parse_tree_res(tree_res);
    const upto_resolution_res res = {
      .success = false,
    };
    return res;
  }
  const resolution_errors errs = resolve_bindings(tree_res.tree, input);
  if (errs.binding_amt > 0) {
    const upto_resolution_res res = {
      .success = false,
    };
    char *error_str = print_resolution_errors_string(input, errs);
    failf(state, "Resolution failed:\n%s", error_str);
    free(error_str);
    free(errs.bindings);
    free_parse_tree_res(tree_res);
    return res;
  } else {
    const upto_resolution_res res = {
      .success = true,
      .tree = tree_res.tree,
    };
    return res;
  }
}

tc_res test_upto_typecheck(test_state *state, const char *restrict input,
                           bool *success, parse_tree *tree) {
  upto_resolution_res tree_res = test_upto_resolution(state, input);
  tc_res tc = {
    .errors = NULL,
    .error_amt = 0,
  };
  if (tree_res.success) {
    tc = typecheck(tree_res.tree);
    add_typecheck_timings(state, tree_res.tree, tc);
    if (tc.error_amt > 0) {
      *success = false;
      stringstream ss;
      ss_init_immovable(&ss);
      print_tc_errors(ss.stream, input, tree_res.tree, tc);
      ss_finalize(&ss);
      failf(state, "Typecheck failed:\n%s", ss.string);
      free(ss.string);
      free_tc_res(tc);
      free_parse_tree(tree_res.tree);
    } else {
      *success = true;
    }
    *tree = tree_res.tree;
  } else {
    *success = false;
  }
  return tc;
}
