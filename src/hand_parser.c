#include <predef/predef/compiler.h>

#include "parse_tree.h"
#include "parser.h"
#include "token.h"
#include "vec.h"

#ifdef __GNUC__
#ifndef LABEL_STACK
#define LABEL_STACK
#endif
#endif

#ifdef LABEL_STACK

#define SAVE_LABEL

#else

typedef enum {
  SJ_STATE_TOPLEVEL,
  SJ_TOPLEVEL_INSIDE_PAREN,
  SJ_FUN_FROM_PARAMS,
  SJ_PATTERN,
} parser_jump;

#endif

typedef struct {
  token *tokens;
  uint32_t ind;
  token t;
} token_state;

VEC_DECL_CUSTOM(void *, vec_ptr);

static void next_token(token_state *state) {
  state->t = state->tokens[state->ind++];
}

/**

  Rules:
  On the way in

*/

parse_tree_res parse(token *tokens, size_t token_amt) {
  token_state state = {
    .tokens = tokens,
    .ind = 0,
  };
  next_token(&state);
  vec_parse_node nodes = VEC_NEW;
  vec_node_ind node_inds = VEC_NEW;
  node_ind_t node_ind = 0;
  parse_node *node;
  parse_node staging;

#ifdef LABEL_STACK
  vec_ptr labels;
#endif

S_STATE_TOPLEVEL:
  VEC_PUSH(&labels, &&S_STATE_TOPLEVEL);
  goto S_BLOCK;

S_BLOCK:
  switch (state.t.type) {
    case TK_OPEN_PAREN:
      goto S_STMT_INSIDE_PAREN;
    default:
      goto ERROR;
  }

S_STMT_INSIDE_PAREN:
  next_token(&state);
  switch (state.t.type) {
    case TK_FUN:
      goto S_FUN_STMT_START;
    default:
      goto ERROR;
  }

S_FUN_STMT_START:
  node = &VEC_DATA_PTR(&nodes)[node_ind];
  node->type.all = PT_ALL_STATEMENT_FUN;
  VEC_PUSH(&nodes, staging);
  VEC_PUSH(&node_inds, node_ind);
  next_token(&state);
  switch (state.t.type) {
    case TK_LOWER_NAME:
      goto S_FUN_STMT_FROM_PARAMS;
    default:
      goto ERROR;
  }

S_FUN_STMT_FROM_PARAMS:
  next_token(&state);
  switch (state.t.type) {
    case TK_OPEN_PAREN:
      goto S_PATTERN;
    default:
      goto ERROR;
  }

S_PATTERN:
  switch (state.t.type) {
    case TK_UPPER_NAME:
      staging.type.all = PT_ALL_PAT_DATA_CONSTRUCTOR_NAME;
      staging.span.start = state.t.start;
      staging.span.len = state.t.len;
      VEC_PUSH(&nodes, staging);
      goto S_PATTERN_IN_PAREN;
    case TK_OPEN_PAREN:
      goto S_PATTERN_IN_PAREN;
    default:
      goto ERROR;
  }

S_PATTERN_IN_PAREN:

ERROR:
  printf("Unexpected token");
  exit(1);
}
