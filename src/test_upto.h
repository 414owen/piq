#pragma once

#include "defs.h"
#include "parse_tree.h"
#include "test.h"
#include "typecheck.h"

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

void add_scanner_timings_internal(test_state *state, const char *restrict input,
                                  tokens_res tres);
void add_parser_timings_internal(test_state *state, tokens_res tres,
                                 parse_tree_res pres);
void add_typecheck_timings_internal(test_state *state, parse_tree_res pres,
                                    tc_res tc_res);

tokens_res test_upto_tokens(test_state *state, const char *input);
parse_tree_res test_upto_parse_tree(test_state *state, const char *input);
tc_res test_upto_typecheck(test_state *state, const char *input, bool *success,
                           parse_tree *tree);
