#include <predef/predef/compiler.h>

#include "parse_tree.h"
#include "token.h"
#include "vec.h"

#define STACKLESS_SPANS 1
#define STACKLESS_PUSH 1

#define parser_ret                                                             \
  {                                                                            \
    void *next_label;                                                          \
    VEC_POP(&state.labels, &next_label);                                       \
    goto *next_label;                                                          \
  }

VEC_DECL_CUSTOM(void *, vec_ptr);

typedef struct {
  token *tokens;
  // token index
  node_ind_t token_ind;
  token t;
  // used to return positions in the node vec
  vec_node_ind ind_stack;
  // backups of the above stack length, to be able to pop runs of indices
  vec_node_ind ind_run_stack;
  vec_ptr labels;

  // parse node data
  vec_parse_node nodes;
  // parse node data
  vec_node_ind node_inds;
} parser_state;

static void parser_consume(parser_state *state) {
  state->t = state->tokens[state->token_ind++];
}

static void parser_return_to(vec_ptr *labels, void *label) {
  VEC_PUSH(labels, label);
}

static void parser_return_to_with_node(parser_state *state, node_ind_t node_ind,
                                       void *label) {
  VEC_PUSH(&state->ind_stack, node_ind);
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
  VEC_PUSH(nodes, staging);                                                    \
  parser_consume(&state);
#else
static void parser_push_primitive(vec_parse_node *nodes, token t,
                                  parse_node_type_all type) {
  parse_node node = {
    .type.all = type,
    .span = span_from_token(t),
  };
  VEC_PUSH(nodes, node);
  parser_consume(&state);
}
#endif

#define TYPE_CASES_GENERIC(stmt)                                               \
  case TKN_OPEN_PAREN:                                                         \
    stmt;                                                                      \
    goto S_TYPE_AT_OPEN_PAREN;                                                 \
  case TKN_LOWER_NAME:                                                         \
    stmt;                                                                      \
    goto S_TYPE_VARIABLE;                                                      \
  case TKN_UPPER_NAME:                                                         \
    stmt;                                                                      \
    goto S_TYPE_CONSTRUCTOR;                                                   \
  case TKN_UNIT:                                                               \
    stmt;                                                                      \
    goto S_TYPE_UNIT;

#define TYPE_CASES_WITH_RETURN(return_label)                                   \
  TYPE_CASES_GENERIC(parser_return_to(&state.labels, return_label))

#define PASS                                                                   \
  do {                                                                         \
  } while (false)

#define TYPE_CASES TYPE_CASES_GENERIC(PASS)

#define PARSER_RESTORE_NODEP                                                   \
  {                                                                            \
    node_ind_t node_ind;                                                       \
    VEC_POP(&state.ind_stack, &node_ind);                                      \
    nodep = &VEC_DATA_PTR(&state.nodes)[node_ind];                             \
  }

#define PARSER_PEEK_NODEP                                                      \
  {                                                                            \
    node_ind_t node_ind = VEC_PEEK(state.ind_stack);                           \
    nodep = &VEC_DATA_PTR(&state.nodes)[node_ind];                             \
  }

static void parser_pop_node_and_ind_and_run(parser_state *state) {
  VEC_POP_(&state->ind_stack);
  VEC_POP_(&state->nodes);
  VEC_POP_(&state->ind_run_stack);
}

#define S_EXPR_OPEN_PAREN_CASE \
  case TKN_OPEN_PAREN:                                                         \
    ON_COMPOUND(S_EXPR_IN_PAREN)                                               \

#define S_EXPR_OPEN_BRACKET_CASE \
  case TKN_OPEN_BRACKET:                                                       \
    ON_COMPOUND(S_EXPR_LIST)

#define S_EXPR_CASES_COMPOUND                                                  \
  S_EXPR_OPEN_PAREN_CASE \
  S_EXPR_OPEN_BRACKET_CASE

#define S_EXPR_CASES_PRIMITIVE                                                 \
  case TKN_INT:                                                                \
    ON_PRIMITIVE(PT_ALL_EX_INT);                                               \
  case TKN_UNIT:                                                               \
    ON_PRIMITIVE(PT_ALL_EX_UNIT);                                              \
  case TKN_STRING:                                                             \
    ON_PRIMITIVE(PT_ALL_EX_STRING);                                            \
  case TKN_LOWER_NAME:                                                         \
    ON_PRIMITIVE(PT_ALL_EX_TERM_NAME);                                         \
  case TKN_UPPER_NAME:                                                         \
    ON_PRIMITIVE(PT_ALL_EX_UPPER_NAME);

#define S_EXPR_CASES                                                           \
  S_EXPR_CASES_COMPOUND                                                        \
  S_EXPR_CASES_PRIMITIVE

#define S_EXPR_CASES_WITHOUT_OPEN_PAREN \
  S_EXPR_CASES_PRIMITIVE \
  S_EXPR_OPEN_BRACKET_CASE

parse_tree_res parse(token *restrict tokens) {
  parser_state state = {
    .tokens = tokens,
    // token index
    .token_ind = 0,
    // stack to restore working node
    .ind_stack = VEC_NEW,
    .ind_run_stack = VEC_NEW,
    .labels = VEC_NEW,

    // parse tree data
    .nodes = VEC_NEW,
    // parse tree data
    .node_inds = VEC_NEW,
  };
  parse_node *nodep;
  parse_node staging;
  struct timespec start = get_monotonic_time();
  parse_node_type_all node_type;

  // consuming empty, setting PEEK to the first token
  parser_consume(&state);

S_STATE_TOPLEVEL:
  parser_return_to(&state.labels, &&S_STATE_TOPLEVEL);
  switch (PEEK.type) {
    case TKN_EOF:
      goto S_SUCCESS;
    default:
      goto S_BLOCK;
  }

S_BLOCK:

S_STMT:
  switch (PEEK.type) {
    case TKN_OPEN_PAREN:
      goto S_STMT_INSIDE_PAREN;
#define ON_COMPOUND(label) goto label;
#define ON_PRIMITIVE(pt_type)                                                  \
  node_type = pt_type;                                                                 \
  break;
    S_EXPR_CASES_WITHOUT_OPEN_PAREN;
#undef ON_COMPOUND
#undef ON_PRIMITIVE
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
      case TKN_LET:
        goto S_LET_STMT_START;
      default:
        goto S_ERROR;
    }

// returns node
S_FUN_STMT_START:
  // fun
  parser_consume(&state);

  staging.type.all = PT_ALL_STATEMENT_FUN;
  parser_return_to_with_node(&state, state.nodes.len, &&S_FUN_STMT_END);
  VEC_PUSH(&state.nodes, staging);
  parser_return_to(&state.labels, &&S_FUN_STMT_END);

  switch (PEEK.type) {
    case TKN_LOWER_NAME:
      goto S_FUN_STMT_AFTER_NAME;
    default:
      goto S_ERROR;
  }

S_LET_STMT_START:
  // let
  parser_consume(&state);
  switch (PEEK.type) {
    case TKN_LOWER_NAME:
      goto S_LET_STMT_AT_NAME;
    default:
      goto S_ERROR;
  }

S_LET_STMT_AT_NAME:
  parser_return_to_with_node(&state, state.nodes.len, &&S_LET_STMT_END);
  staging.sub_a = state.nodes.len;
  VEC_PUSH(&state.nodes, staging);
  parser_push_primitive(&state.nodes, PEEK, PT_ALL_MULTI_TERM_NAME);
  goto S_EXPR;

S_EXPR:
  switch (PEEK.type) {
#define ON_COMPOUND(label) goto label;
#define ON_PRIMITIVE(pt_type)                                                  \
  node_type = pt_type;                                                         \
  break;
    S_EXPR_CASES;
#undef ON_COMPOUND
#undef ON_PRIMITIVE
    default:
      goto S_ERROR;
  }
  goto PARSER_RETURN_PRIMITIVE;

S_EXPR_LIST:
  staging.type.all = PT_ALL_EX_LIST;
  staging.span.start = PEEK.start;
  VEC_PUSH(&state.nodes, staging);
  // open bracket
  parser_consume(&state);
  goto S_EXPR_LIST_NEXT;

  S_EXPR_LIST_NEXT:
    switch (PEEK.type) {
      case TKN_CLOSE_BRACKET:
        goto S_EXPR_LIST_END;
#define ON_COMPOUND(label)                                                     \
  parser_return_to(&state.labels, &&S_EXPR_LIST_NEXT);                         \
  goto label;
#define ON_PRIMITIVE(pt_type)                                                  \
  node_type = pt_type;                                                                 \
  break;
        S_EXPR_CASES;
#undef ON_COMPOUND
#undef ON_PRIMITIVE
      default:
        goto S_ERROR;
    }
    VEC_PUSH(&state.ind_stack, state.nodes.len);
    parser_push_primitive(&state.nodes, PEEK, node_type);
    goto S_EXPR_LIST_NEXT;

S_EXPR_LIST_END:
  // TODO
  parser_ret;

  S_EXPR_IN_PAREN:
    staging.span.start = PEEK.start;
    // open paren
    parser_consume(&state);
    staging.type.all = PT_ALL_EX_CALL;
    VEC_PUSH(&state.ind_stack, state.nodes.len);
    VEC_PUSH(&state.nodes, staging);

    // pushed for compounds
    VEC_PUSH(&state.labels, &&S_CALL);
    VEC_PUSH(&state.ind_run_stack, state.ind_stack.len);

    switch (PEEK.type) {
      case TKN_AS:
        parser_pop_node_and_ind_and_run(&state);
        goto S_AS_EXPR;
      case TKN_FN:
        parser_pop_node_and_ind_and_run(&state);
        goto S_FN_EXPR;
      case TKN_IF:
        parser_pop_node_and_ind_and_run(&state);
        goto S_IF_EXPR;
#define ON_COMPOUND(label) goto label;
#define ON_PRIMITIVE(pt_type)                                                  \
  node_type = pt_type;                                                                 \
  break;
        S_EXPR_CASES;
#undef ON_COMPOUND
#undef ON_PRIMITIVE
      default:
        goto S_ERROR;
    }

  S_CALL_EXPR_PRIM:
    VEC_POP_(&state.labels);
    // for return
    VEC_PUSH(&state.ind_stack, state.nodes.len);
    parser_push_primitive(&state.nodes, PEEK, node_type);

  S_CALL:
    parser_return_to(&state.labels, &&S_CALL);
    switch (PEEK.type) {
      case TKN_CLOSE_PAREN:
        VEC_POP_(&state.labels);
        goto S_CALL_END;
#define ON_COMPOUND(label) goto label;
#define ON_PRIMITIVE(pt_type)                                                  \
  node_type = pt_type;                                                                 \
  goto S_CALL_EXPR_PRIM;
        S_EXPR_CASES;
#undef ON_COMPOUND
#undef ON_PRIMITIVE
      default:
        goto S_ERROR;
    }

  {
    node_ind_t ind_stack_start;
  S_CALL_END:
    // close paren
    parser_consume(&state);
    VEC_POP(&state.ind_run_stack, &ind_stack_start);
    node_ind_t ind_amt = state.ind_stack.len - ind_stack_start;
    VEC_APPEND(&state.node_inds,
               ind_amt,
               &VEC_DATA_PTR(&state.ind_stack)[ind_stack_start]);
    VEC_POP_N(&state.ind_stack, ind_amt);
    PARSER_PEEK_NODEP;
    nodep->sub_amt = ind_amt;
    nodep->subs_start = state.node_inds.len - ind_amt;
  }

S_IF_EXPR:
  // if
  parser_consume(&state);
  staging.type.all = PT_ALL_EX_IF;
  parser_return_to_with_node(&state, state.nodes.len, &&S_IF_EXPR_END);
  VEC_PUSH(&state.nodes, staging);
  // else
  parser_return_to(&state.labels, &&S_EXPR);
  // then
  parser_return_to(&state.labels, &&S_EXPR);
  // cond
  parser_return_to(&state.labels, &&S_EXPR);
  goto S_EXPR;

  {
    node_ind_t cond_ind;
    node_ind_t then_ind;
    node_ind_t else_ind;
  S_IF_EXPR_END:
    // TODO replace with a VEC_POP_N type thing
    VEC_POP(&state.ind_stack, &else_ind);
    VEC_POP(&state.ind_stack, &then_ind);
    VEC_POP(&state.ind_stack, &cond_ind);
    PARSER_PEEK_NODEP;
    nodep->span.len = PEEK.start + PEEK.len - nodep->span.start;
    nodep->subs_start = state.node_inds.len;
    nodep->sub_amt = 3;
    VEC_PUSH(&state.node_inds, cond_ind);
    VEC_PUSH(&state.node_inds, then_ind);
    VEC_PUSH(&state.node_inds, else_ind);
    parser_ret;
  }

S_AS_EXPR:
  parser_return_to_with_node(&state, state.nodes.len, &&S_AS_EXPR_END);
  VEC_PUSH(&state.labels, &&S_EXPR);
  staging.sub_a = state.nodes.len;
  staging.type.all = PT_ALL_EX_AS;
  VEC_PUSH(&state.nodes, staging);
  goto S_TYPE;

  {
    node_ind_t expr_ind;
  S_LET_STMT_END:
    VEC_POP(&state.ind_stack, &expr_ind);
    PARSER_PEEK_NODEP;
    nodep->sub_b = expr_ind;
    nodep->span.len = PEEK.start + PEEK.len - nodep->span.start;
    // close paren
    parser_consume(&state);
    parser_ret;
  }

  {
    node_ind_t expr_ind;
    node_ind_t type_ind;
  S_AS_EXPR_END:
    VEC_POP(&state.ind_stack, &expr_ind);
    VEC_POP(&state.ind_stack, &type_ind);
    PARSER_PEEK_NODEP;
    nodep->sub_a = type_ind;
    nodep->sub_b = expr_ind;
    nodep->span.len = PEEK.start + PEEK.len - nodep->span.start;
    // close paren
    parser_consume(&state);
    parser_ret;
  }

S_FN_EXPR:
  parser_return_to_with_node(&state, state.nodes.len, &&S_FN_EXPR_END);
  // TODO
  goto S_FUNLIKE_AT_PARAMS;

S_SIG_STMT_START:
  parser_return_to_with_node(&state, state.nodes.len, &&S_SIG_END);
  // TODO
  // sig
  parser_consume(&state);
  switch (PEEK.type) {
    case TKN_LOWER_NAME:
      goto S_SIG_AT_BINDING;
    default:
      goto S_ERROR;
  }

S_SIG_AT_BINDING:
  // TODO
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
  parser_push_primitive(&state.nodes, PEEK, PT_ALL_TY_UNIT);
  parser_ret;

S_TYPE_CONSTRUCTOR:
  parser_push_primitive(&state.nodes, PEEK, PT_ALL_TY_CONSTRUCTOR_NAME);
  parser_ret;

S_TYPE_VARIABLE:
  parser_push_primitive(&state.nodes, PEEK, PT_ALL_TY_PARAM_NAME);
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
  goto S_FUNLIKE_AT_PARAMS;

// Either a function statement or a lambda expression
S_FUNLIKE_AT_PARAMS:
  switch (PEEK.type) {
    case TKN_UNIT:
      goto S_FUNLIKE_AFTER_PARAMS;
    case TKN_OPEN_PAREN:
      goto S_NEXT_PARAM;
    default:
      goto S_ERROR;
  }

S_FUNLIKE_AFTER_PARAMS:
  // unit or close paren
  parser_consume(&state);
  // TODO
  parser_ret;

S_NEXT_PARAM : {
  parse_node_type_all node_type;
  parser_return_to(&state.labels, &&S_NEXT_PARAM);
  // comma or open paren
  parser_consume(&state);

#define PRIMITIVE_PATTERN(token, prim_type)                                    \
  case token:                                                                  \
    node_type = prim_type;                                                     \
    break;

#define PRIMITIVE_PATTERN_CASES                                                \
  PRIMITIVE_PATTERN(TKN_LOWER_NAME, PT_ALL_PAT_WILDCARD);                      \
  PRIMITIVE_PATTERN(TKN_UNIT, PT_ALL_PAT_UNIT);                                \
  PRIMITIVE_PATTERN(TKN_INT, PT_ALL_PAT_INT);

  switch (PEEK.type) {
    PRIMITIVE_PATTERN_CASES;
    PRIMITIVE_PATTERN(TKN_UPPER_NAME, PT_ALL_PAT_DATA_CONSTRUCTOR_NAME);
    case TKN_CLOSE_BRACKET:
      goto S_FUNLIKE_AFTER_PARAMS;
    default:
      goto S_ERROR;
  }
  parser_push_primitive(&state.nodes, PEEK, node_type);
  goto S_NEXT_PARAM;
}

S_PATTERN : {
  parse_node_type_all node_type;

  switch (PEEK.type) {
    PRIMITIVE_PATTERN_CASES;
    case TKN_UPPER_NAME:
      goto S_PATTERN_DATA_CONSTRUCTOR;
    case TKN_OPEN_PAREN:
      goto S_PATTERN_AT_OPEN_PAREN;
    default:
      goto S_ERROR;
  }
#undef PRIMITIVE_PATTERN
  parser_push_primitive(&state.nodes, PEEK, node_type);
  parser_ret;
}

S_PATTERN_DATA_CONSTRUCTOR:
  parser_push_primitive(&state.nodes, PEEK, PT_ALL_PAT_DATA_CONSTRUCTOR_NAME);
  parser_ret;

  // TODO benchmark this one as just a normal function too
S_SIG_END:
S_AT_CLOSE_PAREN:
  PARSER_RESTORE_NODEP;
  nodep->span.len = PEEK.start + PEEK.len - nodep->span.start;
  // close paren
  parser_consume(&state);
  parser_ret;

S_FN_EXPR_END:
  PARSER_RESTORE_NODEP;
  nodep->span.len = PEEK.start + PEEK.len - nodep->span.start;
  // TODO add params and expr
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
  VEC_PUSH(&state.nodes, staging);
  parser_ret;

PARSER_RETURN_PRIMITIVE:
  VEC_PUSH(&state.node_inds, state.nodes.len);
  parser_push_primitive(&state.nodes, PEEK, node_type);
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
    res.tree.node_amt = state.nodes.len;
    VEC_FREE(&state.nodes);
    VEC_FREE(&state.node_inds);
    goto S_END;

  S_SUCCESS:
    // should just have toplevel label
    debug_assert(state.labels.len == 1);
    res.type = PRT_SUCCESS;
    res.tree.nodes = VEC_FINALIZE(&state.nodes);
    res.tree.inds = VEC_FINALIZE(&state.node_inds);
    res.tree.node_amt = state.nodes.len;
    res.tree.root_subs_start = 0;
    res.tree.root_subs_amt = 0;
    goto S_END;

  S_END:
    VEC_FREE(&state.labels);
    // shouldn't have leaks here
    // debug_assert(state.ind_stack.len == 1);
    VEC_FREE(&state.ind_stack);
    res.time_taken = time_since_monotonic(start);
    return res;
  }
}
