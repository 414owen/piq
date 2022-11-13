#pragma once

#include "parse_tree.h"
#include "test.h"
#include "typecheck.h"

tokens_res test_upto_tokens(test_state *state, const char *input);
parse_tree_res test_upto_parse_tree(test_state *state, const char *input);
tc_res test_upto_typecheck(test_state *state, const char *input, bool *success,
                           parse_tree *tree);
