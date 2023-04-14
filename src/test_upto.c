// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "defs.h"
#include "diagnostic.h"
#include "externalise_spans.h"
#include "test.h"
#include "test_upto.h"
#include "test_llvm.h"
#include "perf.h"
#include "typedefs.h"

#ifdef TIME_TOKENIZER
// TODO use pre-known file sizes for non-tests
void add_scanner_timings_internal(test_state *state, const char *restrict input,
                                  tokens_res tres) {
  if (tres.succeeded) {
    state->total_bytes_tokenized += strlen(input);
  } else {
    state->total_bytes_tokenized += tres.error_pos;
  }
  state->total_tokenization_perf =
    perf_add(state->total_tokenization_perf, tres.perf_values);
  state->total_tokens_produced += tres.token_amt;
}
#endif

#ifdef TIME_PARSER
void add_parser_timings_internal(test_state *state, tokens_res tres,
                                 parse_tree_res pres) {
  if (pres.success) {
    state->total_tokens_parsed += tres.token_amt;
  } else {
    state->total_tokens_parsed += pres.errors.error_pos;
  }

  state->total_parser_perf =
    perf_add(state->total_parser_perf, pres.perf_values);
  state->total_parse_nodes_produced += pres.tree.node_amt;
}
#endif

#ifdef TIME_TYPECHECK
void add_typecheck_timings_internal(test_state *state, parse_tree tree,
                                    tc_res tc_res) {
  if (tc_res.error_amt == 0) {
    state->total_typecheck_perf =
      perf_add(state->total_typecheck_perf, tc_res.perf_values);
    state->total_parse_nodes_typechecked += tree.node_amt;
  }
}
#endif

#ifdef TIME_CODEGEN
void add_codegen_timings_internal(test_state *state, parse_tree tree,
                                  llvm_res llres) {
  state->total_llvm_ir_generation_perf =
    perf_add(state->total_llvm_ir_generation_perf, llres.perf_values);
  state->total_parse_nodes_codegened += tree.node_amt;
}
#endif

#ifdef TIME_NAME_RESOLUTION
void add_name_resolution_timings_internal(test_state *state,
                                          resolution_res res) {
  state->total_name_resolution_perf =
    perf_add(state->total_name_resolution_perf, res.perf_values);
  state->total_names_looked_up += res.num_names_looked_up;
}
#endif

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
      .success = false,
      .errors =
        {
          .error_pos = 0,
          .expected_amt = 0,
          .expected = NULL,
        },
    };
    failf(state, "Tokenization failed at: %u", tres.error_pos);
    free_tokens_res(tres);
    return res;
  }

  const parse_tree_res pres = parse(tres.tokens, tres.token_amt);

  add_parser_timings(state, tres, pres);

  if (!pres.success) {
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
  if (!tree_res.success) {
    free_parse_tree_res(tree_res);
    const upto_resolution_res res = {
      .success = false,
    };
    return res;
  }
  const resolution_res res_res = resolve_bindings(tree_res.tree, input);

  add_name_resolution_timings(state, res_res);

  if (res_res.not_found.binding_amt > 0) {
    const upto_resolution_res res = {
      .success = false,
    };
    char *error_str = print_resolution_errors_string(input, res_res.not_found);
    failf(state, "Resolution failed:\n%s", error_str);
    free(error_str);
    free(res_res.not_found.bindings);
    free_parse_tree_res(tree_res);
    return res;
  } else {
    externalise_spans(&tree_res.tree);
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

// This one brackets the continuation, so it handles cleanup too
// see
// [bracket](https://hackage.haskell.org/package/base-4.17.0.0/docs/Control-Exception.html#v:bracket)
void test_upto_codegen_with(test_state *state, const char *restrict input,
                            llvm_symbol_test *tests, int num_tests) {
  bool success = true;
  parse_tree tree;
  tc_res tc_res = test_upto_typecheck(state, input, &success, &tree);
  if (!success) {
    return;
  }

  source_file test_file = {
    .path = "test_jit",
    .data = input,
  };

  llvm_jit_ctx ctx = llvm_jit_init();

  LLVMModuleRef module =
    LLVMModuleCreateWithNameInContext(test_file.path, ctx.llvm_ctx);
  llvm_res res = llvm_gen_module(test_file, tree, tc_res.types, module);
  add_codegen_timings(state, tree, res);

  // ctx->module_str = LLVMPrintModuleToString(res.module);
  // puts(ctx->module_str);

#ifdef TIME_CODEGEN
  perf_state perf_state = perf_start();
#endif

  char *mod_str = LLVMPrintModuleToString(module);

  LLVMOrcThreadSafeModuleRef tsm =
    LLVMOrcCreateNewThreadSafeModule(res.module, ctx.orc_ctx);
  LLVMOrcLLJITAddLLVMIRModule(ctx.jit, ctx.dylib, tsm);
  LLVMOrcJITTargetAddress entry_addr = (LLVMOrcJITTargetAddress)NULL;

  char *module_preamble = "\nIn module:\n";
  bool have_printed_module = false;
  for (int i = 0; i < num_tests; i++) {
    llvm_symbol_test test = tests[i];
    LLVMErrorRef error =
      LLVMOrcLLJITLookup(ctx.jit, &entry_addr, test.symbol_name);
    if (error == LLVMErrorSuccess) {
      char *err = test.test_callback((voidfn)entry_addr, test.data);
      if (err != NULL) {
        failf(state,
              "%s\nOn symbol: '%s'%s%s",
              err,
              test.symbol_name,
              module_preamble,
              mod_str);
        if (!have_printed_module) {
          have_printed_module = true;
          module_preamble = "";
          mod_str = "";
        }
      }
    } else {
      char *msg = LLVMGetErrorMessage(error);
      failf(state,
            "LLVMLLJITLookup for symbol '%s' failed:\n%s%s%s",
            test.symbol_name,
            msg,
            module_preamble,
            mod_str);
      if (!have_printed_module) {
        have_printed_module = true;
        module_preamble = "";
        mod_str = "";
      }
      LLVMDisposeErrorMessage(msg);
    }
  }
  LLVMDisposeMessage(mod_str);

#ifdef TIME_CODEGEN
  perf_values perf_values = perf_end(perf_state);
  if (!state->current_failed) {
    state->total_codegen_perf =
      perf_add(state->total_codegen_perf, perf_values);
  }
#endif

  free_parse_tree(tree);
  free_tc_res(tc_res);
  llvm_jit_dispose(&ctx);
}
