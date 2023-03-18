#include <predef/predef/compiler.h>

#include "parse_tree.h"
#include "token.h"
#include "vec.h"

#define STACKLESS_SPANS 1
#define STACKLESS_PUSH 1

#define parser_ret                          \
  {                                         \
    void *next_label;                       \
    VEC_POP(&state.labels, &next_label);    \
    goto *next_label;                       \
  }

VEC_DECL_CUSTOM(void *, vec_ptr);

typedef struct {
  token *tokens;
  // token index
  node_ind_t token_ind;
  token t;
  vec_node_ind node_inds;
  vec_ptr labels;
} parser_state;

static void parser_consume(parser_state *state) {
  state->t = state->tokens[state->token_ind++];
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

#define TYPE_CASES_GENERIC(stmt) \
  case TKN_OPEN_PAREN: \
    stmt; \
    goto S_TYPE_AT_OPEN_PAREN; \
  case TKN_LOWER_NAME: \
    stmt; \
    goto S_TYPE_VARIABLE; \
  case TKN_UPPER_NAME: \
    stmt; \
    goto S_TYPE_CONSTRUCTOR; \
  case TKN_UNIT: \
    stmt; \
    goto S_TYPE_UNIT;

#define TYPE_CASES_WITH_RETURN(return_label) \
  TYPE_CASES_GENERIC(parser_return_to(&state.labels, return_label))

#define PASS do {} while(false)

#define TYPE_CASES TYPE_CASES_GENERIC(PASS)

#define PARSER_RESTORE_NODEP \
  { \
    node_ind_t node_ind; \
    VEC_POP(&state.node_inds, &node_ind); \
    nodep = &VEC_DATA_PTR(&nodes)[node_ind]; \
  }

parse_tree_res parse(token *restrict tokens) {
  parser_state state = {
    .tokens = tokens,
    // token index
    .token_ind = 0,
    // stack to restore working node
    .node_inds = VEC_NEW,
    .labels = VEC_NEW,
  };
  // parse tree data
  vec_parse_node nodes = VEC_NEW;
  // parse tree data
  vec_node_ind inds = VEC_NEW;
  parse_node *nodep;
  parse_node staging;
  struct timespec start = get_monotonic_time();

  // consuming empty, setting PEEK to the first token
  parser_consume(&state);

S_STATE_TOPLEVEL:
  parser_return_to(&state.labels, &&S_STATE_TOPLEVEL);
  goto S_BLOCK;

S_BLOCK:
  switch (PEEK.type) {
    case TKN_OPEN_PAREN:
      goto S_STMT_INSIDE_PAREN;
    case TKN_EOF:
      goto S_SUCCESS;
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
  parser_return_to_with_node(&state, nodes.len, &&S_SIG_END);
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
    default:
      TYPE_CASES;
      goto S_ERROR;
  }

S_TYPE_UNIT:
  parser_push_primitive(&nodes, PEEK, PT_ALL_TY_UNIT);
  // unit
  parser_consume(&state);
  parser_ret;

S_TYPE_CONSTRUCTOR:
  parser_push_primitive(&nodes, PEEK, PT_ALL_TY_CONSTRUCTOR_NAME);
  // binding
  parser_consume(&state);
  parser_ret;

S_TYPE_VARIABLE:
  parser_push_primitive(&nodes, PEEK, PT_ALL_TY_PARAM_NAME);
  // binding
  parser_consume(&state);
  parser_ret;

S_TYPE_AT_OPEN_PAREN:
  // paren
  parser_consume(&state);
  switch (PEEK.type) {
    case TKN_FN_TYPE:
      goto S_FN_TYPE;
    TYPE_CASES_WITH_RETURN(&&S_TYPE_CALL_OR_TUP)
    default:
      goto S_ERROR;
  }

S_TYPE_CALL_OR_TUP:
  // TODO manipulate call or tuple here
  switch (PEEK.type) {
    case TKN_COMMA:
      goto S_TYPE_TUPLE_AT_COMMA;
    TYPE_CASES_WITH_RETURN(&&S_CONSTRUCTION_NEXT);
    case TKN_CLOSE_PAREN:
      goto S_TYPE_CONSTRUCTION_END;
    default:
      goto S_ERROR;
  }

S_CONSTRUCTION_NEXT:
  switch (PEEK.type) {
    TYPE_CASES_WITH_RETURN(&&S_CONSTRUCTION_NEXT);
    case TKN_CLOSE_PAREN:
      goto S_TYPE_CONSTRUCTION_END;
    default:
      goto S_ERROR;
  }

S_TYPE_CONSTRUCTION_END:
  // close paren
  parser_consume(&state);
  parser_ret;

S_TYPE_TUPLE_AT_COMMA:
  // comma
  parser_consume(&state);
  switch (PEEK.type) {
    TYPE_CASES_WITH_RETURN(&&S_TYPE_TUPLE_NEXT);
    default:
      goto S_ERROR;
  }

S_TYPE_TUPLE_NEXT:
  switch (PEEK.type) {
    case TKN_COMMA:
      goto S_TYPE_TUPLE_AT_COMMA;
    case TKN_CLOSE_PAREN:
      goto S_TYPE_TUPLE_END;
    default:
      goto S_ERROR;
  }

S_TYPE_TUPLE_END:
  // TODO manip
  // close paren
  parser_consume(&state);
  parser_ret;

S_FN_TYPE:
  // Fn type
  parser_consume(&state);
  parser_return_to(&state.labels, &&S_FN_TYPE_AFTER_FIRST_ARG);
  goto S_TYPE;

S_FN_TYPE_AFTER_FIRST_ARG:
  switch (PEEK.type) {
    TYPE_CASES_WITH_RETURN(&&S_FN_TYPE_AFTER_FIRST_ARG)
    case TKN_CLOSE_PAREN:
      goto S_FN_TYPE_END;
    default:
      goto S_ERROR;
  }

S_FN_TYPE_END:
  // TODO
  // close paren
  parser_consume(&state);
  parser_ret;

S_FUN_STMT_END:
  // TODO
  parser_ret;

S_FUN_STMT_AFTER_NAME:
  // name
  parser_consume(&state);
  goto S_FUNLIKE_FROM_PARAMS;

// Either a function statement or a lambda expression
S_FUNLIKE_FROM_PARAMS:
  switch (PEEK.type) {
    case TKN_UNIT:
      goto S_FUNLIKE_AFTER_PARAMS;
    case TKN_OPEN_PAREN:
      goto S_FUNLIKE_CONSUME_THEN_PARAMETER;
    default:
      goto S_ERROR;
  }

S_FUNLIKE_AFTER_PARAMS:
  // unit or close paren
  parser_consume(&state);
  // TODO
  parser_ret;

S_NEXT_PARAM:
  parser_consume(&state);
  switch (PEEK.type) {
    case TKN_COMMA:
      goto S_FUNLIKE_CONSUME_THEN_PARAMETER;
    case TKN_CLOSE_BRACKET:
      goto S_FUNLIKE_AFTER_PARAMS;
    default:
      goto S_ERROR;
  }

S_FUNLIKE_CONSUME_THEN_PARAMETER:
  parser_return_to(&state.labels, &&S_NEXT_PARAM);
  // comma or open paren
  parser_consume(&state);
  goto S_PATTERN;

S_PATTERN:
{
  parse_node_type_all node_type;
  switch (PEEK.type) {
#define PRIMITIVE_PATTERN(token, prim_type) \
    case token: \
      node_type = prim_type; \
      break;
    PRIMITIVE_PATTERN(TKN_LOWER_NAME, PT_ALL_PAT_WILDCARD);
    case TKN_UPPER_NAME:
      goto S_PATTERN_DATA_CONSTRUCTOR;
    PRIMITIVE_PATTERN(TKN_UNIT, PT_ALL_PAT_UNIT);
    PRIMITIVE_PATTERN(TKN_INT, PT_ALL_PAT_INT);
    case TKN_OPEN_PAREN:
      goto S_PATTERN_AT_OPEN_PAREN;
    default:
      goto S_ERROR;
  }
#undef PRIMITIVE_PATTERN
  parser_push_primitive(&nodes, PEEK, node_type);
  parser_ret;
}

S_PATTERN_DATA_CONSTRUCTOR:
  parser_push_primitive(&nodes, PEEK, PT_ALL_PAT_DATA_CONSTRUCTOR_NAME);
  parser_ret;

  // TODO benchmark this one as just a normal function too
S_SIG_END:
S_AT_CLOSE_PAREN:
  PARSER_RESTORE_NODEP;
  nodep->span.len = PEEK.start + PEEK.len - nodep->span.start;
  // close paren
  parser_consume(&state);
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

  {
    parse_tree_res res;

S_ERROR:
    res.type = PRT_PARSE_ERROR;
    res.error_pos = state.token_ind - 1;
    // TODO expected tokens
    // This is allocated so we don't segfault on free...
    res.expected = malloc(0);
    res.expected_amt = 0;
    res.tree.node_amt = nodes.len;
    VEC_FREE(&nodes);
    VEC_FREE(&inds);
    goto S_END;

S_SUCCESS:
    // should just have toplevel label
    debug_assert(state.labels.len == 1);
    res.type = PRT_SUCCESS;
    res.tree.nodes = VEC_FINALIZE(&nodes);
    res.tree.inds = VEC_FINALIZE(&inds);
    res.tree.node_amt = nodes.len;
    res.tree.root_subs_start = 0;
    res.tree.root_subs_amt = 0;
    goto S_END;

S_END:
    VEC_FREE(&state.labels);
    // shouldn't have leaks here
    // debug_assert(state.node_inds.len == 1);
    VEC_FREE(&state.node_inds);
    res.time_taken = time_since_monotonic(start);
    return res;
  }
}
