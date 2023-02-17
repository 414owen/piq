#pragma once

#include "defs.h"
#include "parse_tree.h"
#include "test.h"
#include "typecheck.h"
#include "llvm.h"
#include "typedefs.h"

#define pass                                                                   \
  do {                                                                         \
  } while (0)

#ifdef TIME_TOKENIZER
#define add_scanner_timings(...) add_scanner_timings_internal(__VA_ARGS__)
#else
#define add_scanner_timings(...) pass
#endif

#ifdef TIME_PARSER
#define add_parser_timings(...) add_parser_timings_internal(__VA_ARGS__)
#else
#define add_parser_timings(...) pass
#endif

#ifdef TIME_TYPECHECK
#define add_typecheck_timings(...) add_typecheck_timings_internal(__VA_ARGS__)
#else
#define add_typecheck_timings(...) pass
#endif

#ifdef TIME_CODEGEN
#define add_codegen_timings(...) add_codegen_timings_internal(__VA_ARGS__)
#else
#define add_codegen_timings(...) pass
#endif

void add_scanner_timings_internal(test_state *state, const char *restrict input,
                                  tokens_res tres);
void add_parser_timings_internal(test_state *state, tokens_res tres,
                                 parse_tree_res pres);
void add_typecheck_timings_internal(test_state *state, parse_tree tree,
                                    tc_res tc_res);
void add_codegen_timings_internal(test_state *state, parse_tree tree,
                                  llvm_res llres);

tokens_res test_upto_tokens(test_state *state, const char *input);

parse_tree_res test_upto_parse_tree(test_state *state, const char *input);

typedef struct {
  bool success;
  parse_tree tree;
} upto_resolution_res;

upto_resolution_res test_upto_resolution(test_state *state,
                                         const char *restrict input);

tc_res test_upto_typecheck(test_state *state, const char *input, bool *success,
                           parse_tree *tree);

typedef char *test_failure;
typedef test_failure (*symbol_test)(voidfn f, void *data);

typedef struct {
  char *symbol_name;
  symbol_test test_callback;
  void *data;
} llvm_symbol_test;

void test_upto_codegen_with(test_state *state, const char *restrict input,
                            llvm_symbol_test *tests, int num_tests);
