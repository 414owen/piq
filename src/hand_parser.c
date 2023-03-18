#include <predef/predef/compiler.h>

#include "parse_tree.h"
#include "token.h"
#include "vec.h"

#define STACKLESS_SPANS 1
#define STACKLESS_PUSH 1

#define parser_ret                                                             \
  VEC_POP(&state.labels, &next_label);                                         \
  goto *next_label;

VEC_DECL_CUSTOM(void *, vec_ptr);

typedef struct {
  token *tokens;
  node_ind_t ind;
  token t;
  vec_node_ind node_inds;
  vec_ptr labels;
} parser_state;

static void parser_consume(parser_state *state) {
  state->t = state->tokens[state->ind++];
}

static void parser_return_to(vec_ptr *labels, void *label) {
  VEC_PUSH(labels, label);
}

static void parser_return_to_with_node(parser_state *state, node_ind_t node_ind,
                                       void *label) {
  VEC_PUSH(&state->node_inds, node_ind);
  parser_return_to(&state->labels, label);
}

#define PEEK state.t

/**

  Rules:
  On the way in:
  * staging up to date (compound objects have`start` set)
  * we push the current
  * If we meet a compound introducer keyword
    * we push the next step onto the label stack
    * we push our node to the stack
    * Branch to the next step

  In the middle:
  * The current node is on the node stack
  * We pop the index from index stack

  On the way out:

*/

#ifdef STACKLESS_SPANS
#define span_from_token(t)                                                     \
  (span) { .start = t.start, .len = t.len }
#else
static span span_from_token(token t) {
  span res = {
    .start = t.start,
    .len = t.len,
  };
  return res;
}
#endif

#if STACKLESS_PUSH
#define parser_push_primitive(nodes, t, node_type)                             \
  staging.type.all = node_type;                                                \
  staging.span = span_from_token(t);                                           \
  VEC_PUSH(nodes, staging);
#else
static void parser_push_primitive(vec_parse_node *nodes, token t,
                                  parse_node_type_all type) {
  parse_node node = {
    .type.all = type,
    .span = span_from_token(t),
  };
  VEC_PUSH(nodes, node);
}
#endif

parse_tree_res parse(token *restrict tokens) {
  parser_state state = {
    .tokens = tokens,
    .ind = 0,
    .node_inds = VEC_NEW,
    .labels = VEC_NEW,
  };
  vec_parse_node nodes = VEC_NEW;
  parse_node *nodep;
  parse_node staging;
  void *next_label;

  parser_consume(&state);

S_STATE_TOPLEVEL:
  parser_return_to(&state.labels, &&S_STATE_TOPLEVEL);
  goto S_BLOCK;

S_BLOCK:
  switch (PEEK.type) {
    case TKN_OPEN_PAREN:
      goto S_STMT_INSIDE_PAREN;
    case TKN_EOF:
    default:
      goto S_ERROR;
  }

S_STMT_INSIDE_PAREN:
  staging.span.start = PEEK.start;
  // paren
  parser_consume(&state);
  switch (PEEK.type) {
    case TKN_FUN:
      goto S_FUN_STMT_START;
    case TKN_SIG:
      goto S_SIG_STMT_START;
    default:
      goto S_ERROR;
  }

S_FUN_STMT_START:
  // fun
  parser_consume(&state);

  staging.type.all = PT_ALL_STATEMENT_FUN;
  parser_return_to_with_node(&state, nodes.len, &&S_FUN_STMT_END);
  VEC_PUSH(&nodes, staging);
  parser_return_to(&state.labels, &&S_FUN_STMT_END);

  switch (PEEK.type) {
    case TKN_LOWER_NAME:
      goto S_FUN_STMT_AFTER_NAME;
    default:
      goto S_ERROR;
  }

S_SIG_STMT_START:
  // sig
  parser_consume(&state);
  switch (PEEK.type) {
    case TKN_LOWER_NAME:
      goto S_SIG_AT_BINDING;
    default:
      goto S_ERROR;
  }

S_SIG_AT_BINDING:
  parser_push_primitive(&nodes, PEEK, PT_ALL_STATEMENT_SIG);
  // binding
  parser_consume(&state);
  goto S_TYPE;

S_TYPE:
  switch (PEEK.type) {
    case TKN_OPEN_PAREN:
      goto S_TYPE_AT_OPEN_PAREN;
    default:
      goto S_ERROR;
  }

S_TYPE_AT_OPEN_PAREN:
  // paren
  parser_consume(&state);
  switch (PEEK.type) {
    case TKN_UPPER_NAME:
      goto S_TYPE_CONSTRUCTOR;
    default:
      goto S_ERROR;
  }

S_TYPE_CONSTRUCTOR:
  // constructor name
  parser_consume(&state);
  parser_push_primitive(&nodes, PEEK, PT_ALL_PAT_DATA_CONSTRUCTOR_NAME);

S_FUN_STMT_END:
  // TODO

S_FUN_STMT_AFTER_NAME:
  // name
  parser_consume(&state);
  goto S_FUNLIKE_FROM_PARAMS;

// Either a function statement or a lambda expression
S_FUNLIKE_FROM_PARAMS:
  switch (PEEK.type) {
    case TKN_OPEN_PAREN:
      goto S_FUNLIKE_CONSUME_THEN_PARAMETER;
    default:
      goto S_ERROR;
  }

S_NEXT_PARAM:
  parser_consume(&state);
  switch (PEEK.type) {
    case TKN_COMMA:
      goto S_FUNLIKE_CONSUME_THEN_PARAMETER;
    case TKN_CLOSE_BRACKET:
    default:
      goto S_ERROR;
  }

S_FUNLIKE_CONSUME_THEN_PARAMETER:
  parser_return_to(&state.labels, &&S_NEXT_PARAM);
  goto S_CONSUME_THEN_PATTERN;

S_CONSUME_THEN_PATTERN:
  parser_consume(&state);
  goto S_PATTERN;

S_PATTERN:
  switch (PEEK.type) {
    case TKN_UPPER_NAME:
      goto S_PATTERN_DATA_CONSTRUCTOR;
    case TKN_OPEN_PAREN:
      goto S_PATTERN_AT_OPEN_PAREN;
    default:
      goto S_ERROR;
  }

S_PATTERN_DATA_CONSTRUCTOR:
  staging.type.all = PT_ALL_PAT_DATA_CONSTRUCTOR_NAME;
  staging.span.start = PEEK.start;
  staging.span.len = PEEK.len;
  VEC_PUSH(&nodes, staging);
  parser_ret;

  // TODO benchmark this one as just a normal function too
S_AT_CLOSE_PAREN:
  VEC_POP(&state.node_inds, &state.ind);
  nodep = &VEC_DATA_PTR(&nodes)[state.ind];
  nodep->span.len = PEEK.start + PEEK.len - nodep->span.start;
  parser_ret;

S_PATTERN_AT_OPEN_PAREN:
  parser_return_to(&state.labels, &&S_AT_CLOSE_PAREN);
  // paren
  parser_consume(&state);
  switch (PEEK.type) {
    case TKN_UPPER_NAME:
      goto S_PATTERN_DATA_CONSTRUCTOR;
    case TKN_OPEN_PAREN:
      goto S_PATTERN_AT_OPEN_PAREN;
    case TKN_LOWER_NAME:
      goto S_PAT_WILDCARD;
    default:
      goto S_ERROR;
  }

S_PAT_WILDCARD:
  staging.type.all = PT_ALL_PAT_WILDCARD;
  staging.span = span_from_token(PEEK);
  VEC_PUSH(&nodes, staging);
  parser_ret;

S_ERROR : {
  parse_tree_res res = {
    .type = PRT_PARSE_ERROR,
    .error_pos = state.ind - 1,
  };
  return res;
}
}
