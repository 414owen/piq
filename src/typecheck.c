#include <hedley.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "ast_meta.h"
#include "binding.h"
#include "bitset.h"
#include "consts.h"
#include "diagnostic.h"
#include "parse_tree.h"
#include "scope.h"
#include "source.h"
#include "typecheck.h"
#include "util.h"
#include "vec.h"

// Typechecking is implemented as an interpreter for a stack based language
// of typechecking operations.
// This is to avoid stackoverflow on enormous ASTs.

/*

Okay, I took a break from this project, and had to figure out what was going on
here, so here are some notes.

## Auditing

if you set DEBUG_TC, an audit log will be printed to `debug-typechecker`.

If you see an action that can be executed directly, but is instead queued in
`state.actions`, it's probably because we want the action to be auditable
when debugging the typechecker using DEBUG_TC.

## Wanted

A wanted type is a type that needs to match, or the typechecker will error.

## What's the difference between `TC_NODE_UNAMBIGUOUS`, and `TC_NODE_MATCHES`?

`TC_NODE_MATCHES` is for when we have a wanted.
`TC_NODE_UNAMBIGUOUS` is for when we don't have a wanted.

*/

typedef enum {
  TC_POP_N_VARS,
  TC_POP_VARS_TO,
  // terms
  TC_NODE_MATCHES,
  TC_NODE_UNAMBIGUOUS,

  // types
  TC_TYPE,

  // {.from = node index, .to = node index}
  TC_CLONE_ACTUAL_ACTUAL,
  // {.from = node index, .to = node index}
  TC_CLONE_ACTUAL_WANTED,
  // {.from = node index, .to = node index}
  TC_CLONE_WANTED_WANTED,
  // {.from = node index, .to = node index}
  TC_CLONE_WANTED_ACTUAL,

  // {.from = type index, .to = node index}
  TC_WANT_TYPE,
  // {.from = type index, .to = node index}
  TC_ASSIGN_TYPE,

  // If there were errors on top of this node, pop them
  // and execute the action two below
  // Otherwise, execute the action one below
  TC_RECOVER,
} action_tag;

typedef enum {
  TWO_STAGE_ONE = 0,
  TWO_STAGE_TWO = 1,
} two_stage_tag;

typedef enum {
  FOUR_STAGE_ONE = 0,
  FOUR_STAGE_TWO = 1,
  FOUR_STAGE_THREE = 2,
  FOUR_STAGE_FOUR = 3,
} four_stage_tag;

typedef union {
  two_stage_tag two_stage;
  four_stage_tag four_stage;
} stage;

typedef struct {
  action_tag tag;
  union {
    NODE_IND_T amt;
    struct {
      NODE_IND_T node_ind;
      stage stage;
    };
    struct {
      NODE_IND_T from;
      NODE_IND_T to;
    };
  };
} tc_action;

VEC_DECL(tc_action);

typedef struct {
  tc_res res;
  NODE_IND_T current_node_ind;
  stage current_stage;

  // TODO technically it's more efficient to have a stack of NODE_IND_T,
  // and a separate stack of actions. It would also let us `VEC_APPEND` node
  // inds in the rest of this file.
  vec_tc_action stack;

  // type names in scope
  vec_str_ref type_env;
  // ref to source, or builtin
  bitset type_is_builtin;
  // index into res.types
  vec_node_ind type_type_inds;

  // term names in scope
  vec_str_ref term_env;
  // ref to source, or builtin
  bitset term_is_builtin;
  // index into res.types
  vec_node_ind term_type_inds;
  // backup of term env length, for popping values off the environment
  vec_node_ind term_scopes;
  // res.node_types is wanted or known
  NODE_IND_T *wanted;

  NODE_IND_T unknown_ind;
  NODE_IND_T string_type_ind;
  NODE_IND_T bool_type_ind;

  source_file source;
} typecheck_state;

static void push_tc_err(typecheck_state *state, tc_error err) {
  VEC_PUSH(&state->res.errors, err);
}

static NODE_IND_T find_primitive_type(typecheck_state *state, type_tag tag) {
  for (NODE_IND_T i = 0; i < state->res.types.len; i++) {
    type t = VEC_GET(state->res.types, i);
    if (t.tag == tag)
      return i;
  }
  return state->res.types.len;
}

static NODE_IND_T find_type(typecheck_state *state, type_tag tag,
                            NODE_IND_T *subs, NODE_IND_T sub_amt) {
  for (NODE_IND_T i = 0; i < state->res.types.len; i++) {
    type t = VEC_GET(state->res.types, i);
    if (t.tag != tag || t.sub_amt != sub_amt)
      continue;
    NODE_IND_T *p1 = &VEC_DATA_PTR(&state->res.type_inds)[t.subs_start];
    if (memcmp(p1, subs, sub_amt) != 0)
      continue;
    return i;
  }
  return state->res.types.len;
}

static NODE_IND_T mk_primitive_type(typecheck_state *state, type_tag tag) {
  debug_assert(type_repr(tag) == SUBS_NONE);
  NODE_IND_T ind = find_primitive_type(state, tag);
  if (ind < state->res.types.len)
    return ind;
  type t = {
    .tag = tag,
    .sub_amt = 0,
    .subs_start = 0,
  };
  VEC_PUSH(&state->res.types, t);
  return state->res.types.len - 1;
}

static NODE_IND_T mk_type_inline(typecheck_state *state, type_tag tag,
                                 NODE_IND_T sub_a, NODE_IND_T sub_b) {
  debug_assert(type_repr(tag) == SUBS_ONE || type_repr(tag) == SUBS_TWO);
  type t = {
    .tag = tag,
    .sub_a = sub_a,
    .sub_b = sub_b,
  };
  for (size_t i = 0; i < state->res.types.len; i++) {
    type t2 = VEC_GET(state->res.types, i);
    if (inline_types_eq(t, t2)) {
      return i;
    }
  }
  VEC_PUSH(&state->res.types, t);
  return state->res.types.len - 1;
}

static NODE_IND_T mk_type(typecheck_state *state, type_tag tag,
                          NODE_IND_T *subs, NODE_IND_T sub_amt) {
  debug_assert(type_repr(tag) == SUBS_EXTERNAL);
  if (subs == NULL) {
    return mk_primitive_type(state, tag);
  }
  NODE_IND_T ind = find_type(state, tag, subs, sub_amt);
  if (ind < state->res.types.len)
    return ind;
  debug_assert(sizeof(subs[0]) == sizeof(VEC_GET(state->res.type_inds, 0)));
  // size_t find_range(const void *haystack, size_t el_size, size_t el_amt,
  // const void *needle, size_t needle_els);
  ind = find_range(VEC_DATA_PTR(&state->res.type_inds), sizeof(subs[0]),
                   state->res.type_inds.len, subs, sub_amt);
  if (ind == state->res.type_inds.len) {
    VEC_APPEND(&state->res.type_inds, sub_amt, subs);
  }
  type t = {
    .tag = tag,
    .sub_amt = sub_amt,
    .subs_start = ind,
  };
  VEC_PUSH(&state->res.types, t);
  return state->res.types.len - 1;
}

#ifdef DEBUG_TC
static bool is_suspicious_action(typecheck_state *state, tc_action action) {
  switch (action.tag) {
    case TC_POP_N_VARS:
      return action.amt > 100;
    case TC_POP_VARS_TO:
      return action.amt > 10000;
    case TC_NODE_MATCHES:
    case TC_NODE_UNAMBIGUOUS:
    case TC_TYPE:
      return action.node_ind >= state->res.tree.node_amt;
    case TC_CLONE_ACTUAL_ACTUAL:
    case TC_CLONE_ACTUAL_WANTED:
    case TC_CLONE_WANTED_ACTUAL:
    case TC_CLONE_WANTED_WANTED:
      return action.from >= state->res.tree.node_amt ||
             action.to >= state->res.tree.node_amt;
    case TC_ASSIGN_TYPE:
    case TC_WANT_TYPE:
      return action.to >= state->res.tree.node_amt ||
             action.from >= state->res.types.len;
    case TC_RECOVER:
      return state->stack.len < 2;
  }
  return false;
}

static void suspicious_action(tc_action act) {}
#endif

// This is for instrumentation purposes only
static void push_action(typecheck_state *state, tc_action act) {
#ifdef DEBUG_TC
  if (is_suspicious_action(state, act))
    suspicious_action(act);
#endif
  VEC_PUSH(&state->stack, act);
}

static void push_actions(typecheck_state *state, NODE_IND_T amt,
                         const tc_action *actions) {
#ifdef DEBUG_TC
  for (NODE_IND_T i = 0; i < amt; i++) {
    tc_action action = actions[i];
    if (is_suspicious_action(state, action))
      suspicious_action(action);
  }
#endif
  VEC_APPEND(&state->stack, amt, actions);
}

// Add builtin types and custom types to type env
static void setup_type_env(typecheck_state *state) {

  {
    type t = {.tag = T_UNKNOWN, .sub_amt = 0, .subs_start = 0};
    VEC_PUSH(&state->res.types, t);
    state->unknown_ind = state->res.types.len - 1;
  }

  memset_arbitrary(state->res.node_types, &state->unknown_ind,
                   state->res.tree.node_amt, sizeof(NODE_IND_T));
  memset_arbitrary(state->wanted, &state->unknown_ind, state->res.tree.node_amt,
                   sizeof(NODE_IND_T));

  // Prelude
  {
    static const char *builtin_names[] = {
      "Bool", "U8", "U16", "U32", "U64", "I8", "I16", "I32", "I64", "String"};

    static const type_tag primitive_types[] = {
      T_BOOL, T_U8, T_U16, T_U32, T_U64, T_I8, T_I16, T_I32, T_I64};

    VEC_APPEND(&state->type_env, STATIC_LEN(builtin_names), builtin_names);
    NODE_IND_T start_type_ind = state->res.types.len;
    for (NODE_IND_T i = 0; i < STATIC_LEN(primitive_types); i++) {
      VEC_PUSH(&state->type_type_inds, start_type_ind + i);
      type t = {.tag = primitive_types[i], .subs_start = 0, .sub_amt = 0};
      VEC_PUSH(&state->res.types, t);
      bs_push(&state->type_is_builtin, true);
    }

    {
      NODE_IND_T u8_ind = mk_primitive_type(state, T_U8);
      NODE_IND_T list_ind = mk_type_inline(state, T_LIST, u8_ind, 0);
      state->string_type_ind = list_ind;
      VEC_PUSH(&state->type_type_inds, state->res.types.len - 1);
      bs_push(&state->type_is_builtin, true);
    }
    state->bool_type_ind = mk_primitive_type(state, T_BOOL);
  }

  // Declared here
  {
    for (size_t i = 0; i < state->res.tree.node_amt; i++) {
      parse_node node = state->res.tree.nodes[i];
      switch (node.type) {
        // TODO fill in user declared types here
        default:
          break;
      }
    }
  }
}

static bool check_int_fits(typecheck_state *state, parse_node node,
                           uint64_t max) {
  bool res = true;
  const char *buf = state->source.data;
  uint64_t last, n = 0;
  for (size_t i = node.start; i <= node.end; i++) {
    last = n;
    char digit = buf[i];
    n *= 10;
    n += digit - '0';
    if (n < last || n > max) {
      tc_error err = {
        .type = INT_LARGER_THAN_MAX,
        .pos = state->current_node_ind,
      };
      push_tc_err(state, err);
      res = false;
      break;
    }
  }
  return res;
}

static bool check_int_fits_type(typecheck_state *state, NODE_IND_T node_ind,
                                NODE_IND_T wanted_ind) {
  parse_node node = state->res.tree.nodes[node_ind];
  type wanted = VEC_GET(state->res.types, wanted_ind);
  bool fits = true;
  switch (wanted.tag) {
    case T_U8:
      fits = check_int_fits(state, node, UINT8_MAX);
      break;
    case T_U16:
      fits = check_int_fits(state, node, UINT16_MAX);
      break;
    case T_U32:
      fits = check_int_fits(state, node, UINT32_MAX);
      break;
    case T_U64:
      fits = check_int_fits(state, node, UINT64_MAX);
      break;
    case T_I8:
      fits = check_int_fits(state, node, INT8_MAX);
      break;
    case T_I16:
      fits = check_int_fits(state, node, INT16_MAX);
      break;
    case T_I32:
      fits = check_int_fits(state, node, INT32_MAX);
      break;
    case T_I64:
      fits = check_int_fits(state, node, INT64_MAX);
      break;
    default: {
      tc_error err = {
        .type = LITERAL_MISMATCH,
        .pos = node_ind,
        .expected = wanted_ind,
      };
      push_tc_err(state, err);
      break;
    }
  }
  return fits;
}

typedef struct {
  NODE_IND_T a;
  NODE_IND_T b;
} node_ind_tup;

VEC_DECL(node_ind_tup);

/*
static size_t prim_ind(typecheck_state *state, type_tag tag) {
  for (size_t i = 0; i < state->type_env.len; i++) {
    NODE_IND_T type_ind =
      VEC_GET(state->type_type_inds, state->type_type_inds.len - 1 - i);
    if (VEC_GET(state->res.types, type_ind).tag == tag) {
      return type_ind;
    }
  }
  give_up("Can't find primitive type!");
  return 0;
}
*/

static binding node_to_binding(parse_node node) {
  binding b = {.start = node.start, .end = node.end};
  return b;
}

static binding node_to_bnd(parse_node a) {
  binding res = {.start = a.start, .end = a.end};
  return res;
}

// fills in a type from a parse_tree type
static void tc_type(typecheck_state *state) {
  NODE_IND_T node_ind = state->current_node_ind;
  stage stage = state->current_stage;
  parse_node node = state->res.tree.nodes[node_ind];
  NODE_IND_T *inds = state->res.tree.inds;

  switch (node.type) {
    case PT_LIST_TYPE: {
      switch (stage.two_stage) {
        // Typecheck subs
        case TWO_STAGE_ONE: {
          {
            tc_action action = {.tag = TC_TYPE,
                                .node_ind = PT_LIST_TYPE_SUB(node),
                                .stage = {.two_stage = TWO_STAGE_ONE}};
            push_action(state, action);
          }
          tc_action action = {.tag = TC_TYPE,
                              .node_ind = node_ind,
                              .stage = {.two_stage = TWO_STAGE_TWO}};
          push_action(state, action);
          break;
        }
        case TWO_STAGE_TWO: {
          break;
        }
      }
      break;
    }
    case PT_TUP: {
      switch (stage.two_stage) {
        // Typecheck subs
        case TWO_STAGE_ONE: {
          for (NODE_IND_T i = 0; i < PT_TUP_SUB_AMT(node); i++) {
            NODE_IND_T sub_ind = PT_TUP_SUB_IND(inds, node, i);
            tc_action action = {.tag = TC_TYPE,
                                .node_ind = sub_ind,
                                .stage = {.two_stage = TWO_STAGE_ONE}};
            push_action(state, action);
          }
          tc_action action = {.tag = TC_TYPE,
                              .node_ind = node_ind,
                              .stage = {.two_stage = TWO_STAGE_TWO}};
          push_action(state, action);
          NODE_IND_T sub_type_ind =
            state->res.node_types[PT_LIST_TYPE_SUB(node)];
          state->res.node_types[node_ind] =
            mk_type_inline(state, T_TUP, sub_type_ind, sub_type_ind);
          break;
        }
        // Combine
        case TWO_STAGE_TWO: {
          switch (PT_TUP_SUB_AMT(node)) {
            case 0:
              give_up("Zero-length tuple!");
              break;
            case 1:
              give_up("Unary tuples are forbidden!");
              break;
            default: {
              size_t sub_bytes = sizeof(NODE_IND_T) * PT_TUP_SUB_AMT(node);
              NODE_IND_T *sub_type_inds = stalloc(sub_bytes);
              for (NODE_IND_T i = 0; i < PT_TUP_SUB_AMT(node); i++) {
                sub_type_inds[i] =
                  state->res.node_types[PT_TUP_SUB_IND(inds, node, i)];
              }
              state->res.node_types[node_ind] =
                mk_type(state, T_TUP, sub_type_inds, PT_TUP_SUB_AMT(node));
              stfree(sub_type_inds, sub_bytes);
              break;
            }
          }
          break;
        }
      }
      break;
    }
    case PT_LIST: {
      switch (stage.two_stage) {
        // Typecheck sub
        case TWO_STAGE_ONE: {
          NODE_IND_T sub_ind = PT_LIST_TYPE_SUB(node);
          {
            tc_action action = {.tag = TC_TYPE,
                                .node_ind = sub_ind,
                                .stage = {.two_stage = TWO_STAGE_ONE}};
            push_action(state, action);
          }
          tc_action action = {.tag = TC_TYPE,
                              .node_ind = node_ind,
                              .stage = {.two_stage = TWO_STAGE_TWO}};
          push_action(state, action);
          break;
        }
        // Combine
        case TWO_STAGE_TWO: {
          NODE_IND_T *subs = stalloc(node.sub_amt);
          state->res.node_types[node_ind] = mk_type_inline(
            state, T_LIST, state->res.node_types[PT_LIST_TYPE_SUB(node)], 0);
          stfree(subs, node.sub_amt);
          break;
        }
      }
      break;
    }
    case PT_FN_TYPE: {
      NODE_IND_T param_ind = PT_FN_TYPE_PARAM_IND(node);
      NODE_IND_T return_ind = PT_FN_TYPE_RETURN_IND(node);
      switch (stage.two_stage) {
        // Typecheck subs
        case TWO_STAGE_ONE: {
          tc_action actions[] = {
            {.tag = TC_TYPE,
             .node_ind = param_ind,
             .stage = {.two_stage = TWO_STAGE_ONE}},
            {.tag = TC_TYPE,
             .node_ind = return_ind,
             .stage = {.two_stage = TWO_STAGE_ONE}},
            {.tag = TC_TYPE,
             .node_ind = node_ind,
             .stage = {.two_stage = TWO_STAGE_TWO}},
          };
          push_actions(state, STATIC_LEN(actions), actions);
          break;
        }
        // Combine
        case TWO_STAGE_TWO: {
          state->res.node_types[node_ind] =
            mk_type_inline(state, T_FN, state->res.node_types[param_ind],
                           state->res.node_types[return_ind]);
          break;
        }
      }
      break;
    }
    case PT_UPPER_NAME: {
      binding b = {.start = node.start, .end = node.end};
      size_t ind = lookup_bnd(state->source.data, state->type_env,
                              state->type_is_builtin, b);
      if (ind == state->type_env.len) {
        tc_error err = {
          .type = TYPE_NOT_FOUND,
          .pos = node_ind,
        };
        push_tc_err(state, err);
        break;
      }
      NODE_IND_T type_ind = VEC_GET(state->type_type_inds, ind);
      state->res.node_types[node_ind] = type_ind;
      break;
    }
    case PT_UNIT: {
      NODE_IND_T type_ind = mk_primitive_type(state, T_UNIT);
      state->res.node_types[node_ind] = type_ind;
      break;
    }
    case PT_CALL: {
      NODE_IND_T callee_ind = PT_CALL_CALLEE_IND(node);
      NODE_IND_T param_ind = PT_CALL_PARAM_IND(node);
      switch (stage.two_stage) {
        case TWO_STAGE_ONE: {
          tc_action actions[] = {
            {.tag = TC_TYPE,
             .node_ind = callee_ind,
             .stage = {.two_stage = TWO_STAGE_ONE}},
            {.tag = TC_TYPE,
             .node_ind = param_ind,
             .stage = {.two_stage = TWO_STAGE_ONE}},
            {.tag = TC_TYPE,
             .node_ind = node_ind,
             .stage = {.two_stage = TWO_STAGE_ONE}},
          };
          push_actions(state, STATIC_LEN(actions), actions);
          break;
        }
        case TWO_STAGE_TWO: {
          NODE_IND_T callee_type_ind = state->res.node_types[callee_ind];
          type callee = VEC_GET(state->res.types, callee_type_ind);
          switch (callee.tag) {
            case T_UNKNOWN:
              break;
            case T_FN: {
              NODE_IND_T callee_return_type_ind = T_FN_RET_IND(callee);
              state->res.node_types[node_ind] = callee_return_type_ind;
              break;
            }
            default: {
              NODE_IND_T from_to[2] = {state->unknown_ind, state->unknown_ind};
              NODE_IND_T exp =
                mk_type(state, T_FN, from_to, STATIC_LEN(from_to));
              tc_error err = {.type = CALLED_NON_FUNCTION,
                              .pos = node_ind,
                              .expected = exp,
                              .got = callee_ind};
              push_tc_err(state, err);
              break;
            }
          }
          break;
        }
      }
      break;
    }
    case PT_CONSTRUCTION:
    case PT_FN:
    case PT_FUN:
    case PT_FUN_BODY:
    case PT_IF:
    case PT_INT:
    case PT_LOWER_NAME:
    case PT_ROOT:
    case PT_STRING:
    case PT_AS:
    case PT_SIG:
    case PT_LET:
      give_up("Tried creating type from unsupported node");
      break;
  }
}

static bool can_propagate_type(typecheck_state *state, parse_node from,
                               parse_node to) {
  NODE_IND_T a_bnd_ind;
  NODE_IND_T b_bnd_ind;
  switch (to.type) {
    case PT_LET:
      a_bnd_ind = PT_LET_BND_IND(to);
      switch (from.type) {
        case PT_SIG:
          b_bnd_ind = PT_SIG_BINDING_IND(from);
          break;
        default:
          return false;
      }
      break;
    case PT_FUN:
      a_bnd_ind = PT_FUN_BINDING_IND(state->res.tree.inds, to);
      switch (from.type) {
        case PT_SIG:
          b_bnd_ind = PT_SIG_BINDING_IND(from);
          break;
        case PT_FUN:
          b_bnd_ind = PT_FUN_BINDING_IND(state->res.tree.inds, from);
          break;
        default:
          return false;
      }
      break;
    default:
      return false;
  }
  return compare_bnds(state->source.data,
                      node_to_bnd(state->res.tree.nodes[a_bnd_ind]),
                      node_to_bnd(state->res.tree.nodes[b_bnd_ind])) == EQ;
}

static void push_tc_sig(typecheck_state *state, NODE_IND_T node_ind,
                        parse_node node) {
  NODE_IND_T name_ind = PT_SIG_BINDING_IND(node);
  NODE_IND_T type_ind = PT_SIG_TYPE_IND(node);
  tc_action actions[] = {
    {.tag = TC_TYPE,
     .node_ind = type_ind,
     .stage = {.two_stage = TWO_STAGE_ONE}},
    {.tag = TC_CLONE_ACTUAL_ACTUAL, .from = type_ind, .to = name_ind},
    {.tag = TC_CLONE_ACTUAL_ACTUAL, .from = type_ind, .to = node_ind},
  };
  push_actions(state, STATIC_LEN(actions), actions);
}

// enforce sigs => we're root level
static void typecheck_block(typecheck_state *state, bool enforce_sigs) {
  NODE_IND_T node_ind = state->current_node_ind;
  parse_node node = state->res.tree.nodes[node_ind];
  NODE_IND_T wanted_ind = state->wanted[node_ind];
  bool is_wanted = wanted_ind != state->unknown_ind;
  bool can_continue = true;

  char *sub_has_wanted = stcalloc(BITNSLOTS(node.sub_amt), 1);

  NODE_IND_T last_el_ind =
    state->res.tree.inds[node.subs_start + node.sub_amt - 1];

  size_t start_action_amt = state->stack.len;

  if (is_wanted) {
    tc_action action = {
      .tag = TC_CLONE_WANTED_WANTED, .from = node_ind, .to = last_el_ind};
    push_action(state, action);
    bs_data_set(sub_has_wanted, node.sub_amt - 1, true);
  }

  if (node.sub_amt == 0)
    return;

  {
    NODE_IND_T sub_ind = state->res.tree.inds[node.subs_start];
    parse_node sub = state->res.tree.nodes[sub_ind];
    if (sub.type == PT_SIG) {
      push_tc_sig(state, sub_ind, sub);
    } else if (enforce_sigs) {
      tc_error err = {
        .type = NEED_SIGNATURE,
        .pos = sub_ind,
      };
      push_tc_err(state, err);
      can_continue = false;
    }
  }

  // TODO: Should we error on extraneous sigs here?
  for (NODE_IND_T sub_i = 1; sub_i < node.sub_amt; sub_i++) {
    NODE_IND_T sub_ind = state->res.tree.inds[node.subs_start + sub_i];
    parse_node sub = state->res.tree.nodes[sub_ind];
    switch (sub.type) {
      case PT_SIG: {
        push_tc_sig(state, sub_ind, sub);
        break;
      }
      case PT_LET:
      case PT_FUN: {
        NODE_IND_T prev_ind = state->res.tree.inds[node.subs_start + sub_i - 1];
        parse_node prev = state->res.tree.nodes[prev_ind];
        if (can_propagate_type(state, prev, sub)) {
          tc_action action = {
            .tag = TC_CLONE_ACTUAL_WANTED, .from = prev_ind, .to = sub_ind};
          push_action(state, action);
          bs_data_set(sub_has_wanted, sub_i, true);
        } else if (enforce_sigs) {
          tc_error err = {
            .type = NEED_SIGNATURE,
            .pos = sub_ind,
          };
          push_tc_err(state, err);
          can_continue = false;
        }
      }
      default:
        break;
    }
  }

  if (!can_continue) {
    // remove actions
    state->stack.len = start_action_amt;
    goto cleanup_tc_block;
  }

  NODE_IND_T bnd_amt = 0;
  for (NODE_IND_T i = 0; i < node.sub_amt; i++) {
    NODE_IND_T sub_ind = state->res.tree.inds[node.subs_start + i];
    parse_node sub = state->res.tree.nodes[sub_ind];
    switch (sub.type) {
      case PT_SIG:
        break;
      case PT_LET:
      case PT_FUN:
        bnd_amt++;
        HEDLEY_FALL_THROUGH;
      default: {
        action_tag tag = bs_data_get(sub_has_wanted, i) ? TC_NODE_MATCHES
                                                        : TC_NODE_UNAMBIGUOUS;
        tc_action action = {.tag = tag,
                            .node_ind = sub_ind,
                            .stage = {.two_stage = TWO_STAGE_ONE}};
        push_action(state, action);
        break;
      }
    }
  }
  tc_action actions[] = {
    {.tag = TC_POP_N_VARS, .amt = bnd_amt},
    {.tag = TC_CLONE_ACTUAL_ACTUAL, .from = last_el_ind, .to = node_ind}};
  push_actions(state, STATIC_LEN(actions), actions);

cleanup_tc_block:
  stfree(sub_has_wanted, BITNSLOTS(node.sub_amt));
}

typedef struct {
  NODE_IND_T node_ind;
  stage stage;
  parse_node node;
  NODE_IND_T wanted_ind;
  type wanted;
} tc_node_params;

static void tc_as(typecheck_state *state, tc_node_params params) {
  NODE_IND_T val_node_ind = PT_AS_TYPE_IND(params.node);
  NODE_IND_T type_node_ind = PT_AS_VAL_IND(params.node);
  tc_action actions[] = {
    {.tag = TC_TYPE,
     .node_ind = type_node_ind,
     .stage = {.two_stage = TWO_STAGE_ONE}},
    {.tag = TC_CLONE_ACTUAL_ACTUAL, .from = type_node_ind, .to = val_node_ind},
    {.tag = TC_CLONE_ACTUAL_ACTUAL,
     .from = type_node_ind,
     .to = params.node_ind},
    {.tag = TC_NODE_MATCHES,
     .node_ind = val_node_ind,
     .stage = {.two_stage = TWO_STAGE_ONE}},
  };
  push_actions(state, STATIC_LEN(actions), actions);
}

// The triple-switch here isn't ideal.
// Can we remove some while keeping the flow top-to-buttom.
// I think we can, if we extract this into functions, and add call-specific
// action-tags. The functions will be in the right order.
static void tc_call(typecheck_state *state, tc_node_params params) {
  NODE_IND_T callee_ind = PT_CALL_CALLEE_IND(params.node);
  parse_node callee = state->res.tree.nodes[callee_ind];
  NODE_IND_T param_ind = PT_CALL_PARAM_IND(params.node);

  // used in shared blocks
  stage next_stage;

  switch (callee.type) {
    case PT_FN:
      switch (params.stage.two_stage) {
        case TWO_STAGE_ONE: {
          tc_action actions[] = {
            {.tag = TC_NODE_UNAMBIGUOUS,
             .node_ind = param_ind,
             .stage = {.two_stage = TWO_STAGE_ONE}},
            {.tag = TC_NODE_MATCHES,
             .node_ind = params.node_ind,
             .stage = {.two_stage = TWO_STAGE_TWO}},
          };
          push_actions(state, STATIC_LEN(actions), actions);
          break;
        }
        case TWO_STAGE_TWO:
          goto post_tc_param;
      }
      break;
    case PT_UPPER_NAME:
      switch (params.stage.two_stage) {
        case TWO_STAGE_ONE:
          next_stage.two_stage = TWO_STAGE_TWO;
          goto push_tc_callee;
        case TWO_STAGE_TWO:
          goto post_tc_callee;
      }
      break;
    default:
      switch (params.stage.four_stage) {
        case FOUR_STAGE_ONE: {
          tc_action actions[] = {
            {.tag = TC_NODE_UNAMBIGUOUS,
             .node_ind = param_ind,
             .stage = {.two_stage = TWO_STAGE_ONE}},
            {.tag = TC_RECOVER, .amt = state->res.errors.len},
            {.tag = TC_NODE_MATCHES,
             .node_ind = params.node_ind,
             .stage = {.four_stage = FOUR_STAGE_TWO}},
            {.tag = TC_NODE_MATCHES,
             .node_ind = params.node_ind,
             .stage = {.four_stage = FOUR_STAGE_THREE}}};
          push_actions(state, STATIC_LEN(actions), actions);
          break;
        }
        case FOUR_STAGE_TWO:
          goto post_tc_param;

        // typechecking param failed
        case FOUR_STAGE_THREE:
          next_stage.four_stage = FOUR_STAGE_FOUR;
          goto push_tc_callee;
        case FOUR_STAGE_FOUR:
          goto post_tc_callee;
      }
      break;
  }
  return;

push_tc_callee : {
  tc_action actions[] = {
    {.tag = TC_NODE_UNAMBIGUOUS,
     .node_ind = callee_ind,
     .stage = {.two_stage = TWO_STAGE_TWO}},
    {.tag = TC_NODE_MATCHES, .node_ind = params.node_ind, .stage = next_stage},
  };
  push_actions(state, STATIC_LEN(actions), actions);
  return;
}
post_tc_param : {
  NODE_IND_T wanted_fn_type = mk_type_inline(
    state, T_FN, state->res.node_types[param_ind], params.wanted_ind);
  tc_action actions[] = {
    {.tag = TC_WANT_TYPE, .from = wanted_fn_type, .to = callee_ind},
    // Can't be more specific, as the current node might be wanted=unknown
    {.tag = TC_NODE_MATCHES,
     .node_ind = callee_ind,
     .stage = {.two_stage = TWO_STAGE_ONE}},
  };
  push_actions(state, STATIC_LEN(actions), actions);
  return;
}
post_tc_callee : {
  NODE_IND_T callee_type_ind = state->res.node_types[callee_ind];
  type callee_type = VEC_GET(state->res.types, callee_type_ind);
  switch (callee_type.tag) {
    case T_FN: {
      NODE_IND_T callee_param_type_ind = T_FN_RET_IND(callee_type);
      NODE_IND_T callee_ret_type_ind = T_FN_RET_IND(callee_type);
      if (callee_ret_type_ind != params.wanted_ind) {
        tc_error err = {.type = TYPE_MISMATCH,
                        .expected = params.wanted_ind,
                        .got = callee_ret_type_ind,
                        .pos = params.node_ind};
        push_tc_err(state, err);
        break;
      }
      tc_action actions[] = {
        {.tag = TC_WANT_TYPE, .to = param_ind, .from = callee_param_type_ind},
        {.tag = TC_NODE_MATCHES,
         .node_ind = param_ind,
         .stage = {.two_stage = TWO_STAGE_TWO}},
      };
      push_actions(state, STATIC_LEN(actions), actions);
      break;
    }
    default: {
      tc_error err = {
        .type = CALLED_NON_FUNCTION,
        .got = callee_type_ind,
        .pos = callee_ind,
      };
      push_tc_err(state, err);
      break;
    }
  }
  return;
}
}

// Here, we're typechecking a node against 'wanted' (an explicit type given to
// the node) Hence, we shouldn't need a combine two_stage, we just assign the
// 'wanted' type.
static void tc_node_matches(typecheck_state *state, tc_node_params params) {
  tc_action action = {.tag = TC_CLONE_WANTED_ACTUAL,
                      .from = params.node_ind,
                      .to = params.node_ind};
  push_action(state, action);

  switch (params.node.type) {
    case PT_SIG:
    case PT_LET:
      give_up("Unexpected non-block-level construct");
      break;
    case PT_FUN_BODY:
      typecheck_block(state, false);
      break;
    case PT_ROOT:
      typecheck_block(state, true);
      break;
    case PT_UNIT:
      if (params.wanted.tag != T_UNIT) {
        tc_error err = {
          .type = LITERAL_MISMATCH,
          .pos = params.node_ind,
          .expected = params.wanted_ind,
        };
        push_tc_err(state, err);
      }
      break;
    case PT_CONSTRUCTION:
      UNIMPLEMENTED("Typechecking constructors");
      break;
    case PT_STRING:
      if (params.wanted_ind != state->string_type_ind) {
        tc_error err = {
          .type = LITERAL_MISMATCH,
          .pos = params.node_ind,
          .expected = params.wanted_ind,
        };
        push_tc_err(state, err);
      }
      break;
    case PT_LIST: {
      if (params.wanted.tag != T_LIST) {
        tc_error err = {
          .type = LITERAL_MISMATCH,
          .pos = params.node_ind,
          .expected = params.wanted_ind,
        };
        push_tc_err(state, err);
        break;
      }
      NODE_IND_T wanted_sub_ind = T_LIST_SUB_IND(params.wanted);
      for (NODE_IND_T i = 0; i < PT_LIST_SUB_AMT(params.node); i++) {
        NODE_IND_T sub_ind =
          PT_LIST_SUB_IND(state->res.tree.inds, params.node, i);
        tc_action actions[] = {
          {.tag = TC_WANT_TYPE, .from = wanted_sub_ind, .to = sub_ind},
          {.tag = TC_NODE_MATCHES,
           .node_ind = sub_ind,
           .stage = {.two_stage = TWO_STAGE_ONE}}};
        push_actions(state, STATIC_LEN(actions), actions);
      }
      break;
    }
    case PT_AS: {
      switch (params.stage.two_stage) {
        case TWO_STAGE_ONE:
          tc_as(state, params);
          tc_action action = {.tag = TC_NODE_MATCHES,
                              .node_ind = params.node_ind,
                              .stage = {.two_stage = TWO_STAGE_TWO}};
          push_action(state, action);
          break;
        case TWO_STAGE_TWO: {
          NODE_IND_T sig_ind = PT_AS_TYPE_IND(params.node);
          NODE_IND_T sig_type_ind = state->res.node_types[sig_ind];
          if (params.wanted_ind != sig_type_ind) {
            tc_error err = {
              .type = TYPE_MISMATCH,
              .pos = params.node_ind,
              .expected = params.wanted_ind,
              .got = sig_type_ind,
            };
            push_tc_err(state, err);
          }
          break;
        }
      }
      break;
    }
    case PT_TUP:
      if (params.wanted.tag != T_TUP ||
          T_TUP_SUB_AMT(params.wanted) != PT_TUP_SUB_AMT(params.node)) {
        tc_error err = {
          .type = TYPE_HEAD_MISMATCH,
          .pos = params.node_ind,
          .expected = params.wanted_ind,
          .got_type_head = T_TUP,
          .got_arity = PT_TUP_SUB_AMT(params.node),
        };
        push_tc_err(state, err);
        state->stack.len -= params.node.sub_amt + 1;
        break;
      }
      for (NODE_IND_T i = 0; i < PT_TUP_SUB_AMT(params.node); i++) {
        NODE_IND_T sub_ind =
          PT_TUP_SUB_IND(state->res.tree.inds, params.node, i);
        tc_action actions[] = {
          {
            .tag = TC_WANT_TYPE,
            .from = T_TUP_SUB_IND(state->res.type_inds.data, params.wanted, i),
            .to = sub_ind,
          },
          {
            .tag = TC_NODE_MATCHES,
            .node_ind = sub_ind,
            .stage = {.two_stage = TWO_STAGE_ONE},
          }};
        push_actions(state, STATIC_LEN(actions), actions);
      }
      break;
    case PT_IF: {
      NODE_IND_T cond = PT_IF_COND_IND(state->res.tree.inds, params.node);
      NODE_IND_T b1 = PT_IF_A_IND(state->res.tree.inds, params.node);
      NODE_IND_T b2 = PT_IF_B_IND(state->res.tree.inds, params.node);

      const tc_action actions[] = {
        {.tag = TC_WANT_TYPE, .from = state->bool_type_ind, .to = cond},
        {.tag = TC_NODE_MATCHES,
         .node_ind = cond,
         .stage = {.two_stage = TWO_STAGE_ONE}},
        {.tag = TC_CLONE_WANTED_WANTED, .from = params.node_ind, .to = b1},
        {.tag = TC_NODE_MATCHES,
         .node_ind = b1,
         .stage = {.two_stage = TWO_STAGE_ONE}},
        // second branch wanted == first branch
        {.tag = TC_CLONE_ACTUAL_WANTED, .from = b1, .to = b2},
        {.tag = TC_NODE_MATCHES,
         .node_ind = b2,
         .stage = {.two_stage = TWO_STAGE_ONE}},
      };

      push_actions(state, STATIC_LEN(actions), actions);
      break;
    }

    case PT_INT: {
      check_int_fits_type(state, params.node_ind, params.wanted_ind);
      break;
    }

    case PT_UPPER_NAME:
    case PT_LOWER_NAME: {
      binding b = node_to_binding(params.node);
      size_t ind = lookup_bnd(state->source.data, state->term_env,
                              state->term_is_builtin, b);
      if (ind == state->term_env.len) {
        tc_error err = {
          .type = BINDING_NOT_FOUND,
          .pos = params.node_ind,
        };
        push_tc_err(state, err);
        break;
      }
      NODE_IND_T term_ind = VEC_GET(state->term_type_inds, ind);
      NODE_IND_T type_ind = state->res.node_types[term_ind];
      if (params.wanted_ind != state->res.node_types[term_ind]) {
        tc_error err = {
          .type = TYPE_MISMATCH,
          .pos = params.node_ind,
          .expected = params.wanted_ind,
          .got = type_ind,
        };
        push_tc_err(state, err);
      }
      break;
    }

    case PT_FN: {
      NODE_IND_T param_ind = PT_FN_PARAM_IND(params.node);
      NODE_IND_T body_ind = PT_FN_BODY_IND(params.node);
      if (params.wanted.tag == T_FN) {
        tc_action actions[] = {
          {.tag = TC_WANT_TYPE,
           .from = T_FN_PARAM_IND(params.wanted),
           .to = param_ind},
          {.tag = TC_NODE_MATCHES,
           .node_ind = param_ind,
           .stage = {.two_stage = TWO_STAGE_ONE}},
          {.tag = TC_WANT_TYPE,
           .from = T_FN_RET_IND(params.wanted),
           .to = body_ind},
          {.tag = TC_NODE_MATCHES,
           .node_ind = body_ind,
           .stage = {.two_stage = TWO_STAGE_ONE}},
          {.tag = TC_POP_VARS_TO, .amt = state->term_env.len}};
        push_actions(state, STATIC_LEN(actions), actions);
        break;
      }
      tc_error err = {
        .type = TYPE_HEAD_MISMATCH,
        .expected = params.wanted_ind,
        .got_type_head = T_FN,
        .pos = params.node_ind,
      };
      push_tc_err(state, err);
      break;
    }

    case PT_FUN: {
      NODE_IND_T bnd_ind =
        PT_FUN_BINDING_IND(state->res.tree.inds, params.node);
      NODE_IND_T param_ind =
        PT_FUN_PARAM_IND(state->res.tree.inds, params.node);
      NODE_IND_T body_ind = PT_FUN_BODY_IND(state->res.tree.inds, params.node);

      if (params.wanted.tag == T_FN) {
        tc_action actions[] = {
          {.tag = TC_CLONE_ACTUAL_ACTUAL,
           .from = params.node_ind,
           .to = bnd_ind},
          {.tag = TC_WANT_TYPE,
           .from = T_FN_PARAM_IND(params.wanted),
           .to = param_ind},
          {.tag = TC_NODE_MATCHES,
           .node_ind = param_ind,
           .stage = {.two_stage = TWO_STAGE_ONE}},
          {.tag = TC_WANT_TYPE,
           .from = T_FN_RET_IND(params.wanted),
           .to = body_ind},
          {.tag = TC_NODE_MATCHES,
           .node_ind = body_ind,
           .stage = {.two_stage = TWO_STAGE_ONE}},
          {.tag = TC_POP_VARS_TO, .amt = state->term_env.len},
        };
        push_actions(state, STATIC_LEN(actions), actions);
        break;
      }
      tc_error err = {.type = TYPE_HEAD_MISMATCH,
                      .expected = params.wanted_ind,
                      .got_type_head = T_FN,
                      .pos = params.node_ind};
      push_tc_err(state, err);
      break;
    }

    case PT_CALL: {
      tc_call(state, params);
      break;
    }

    case PT_LIST_TYPE:
    case PT_FN_TYPE:
      give_up("Type node at term-level");
      break;
  }
}

// We have no wanted information, so ambiguity will error.
// Used to assigned types based on syntax.
static void tc_node_unambiguous(typecheck_state *state, tc_node_params params) {
  switch (params.node.type) {
    case PT_SIG:
    case PT_LET: {
      give_up("Unexpected non-block-level construct");
      break;
    }
    case PT_FUN_BODY:
      typecheck_block(state, false);
      break;
    case PT_ROOT:
      typecheck_block(state, true);
      break;
    case PT_UNIT: {
      state->res.node_types[params.node_ind] = mk_primitive_type(state, T_UNIT);
      break;
    }
    case PT_CONSTRUCTION: {
      UNIMPLEMENTED("Typechecking constructors");
      break;
    }
    case PT_STRING: {
      state->res.node_types[params.node_ind] = state->string_type_ind;
      break;
    }
    case PT_LIST: {
      NODE_IND_T *inds = state->res.tree.inds;
      NODE_IND_T sub_amt = PT_LIST_SUB_AMT(params.node);
      switch (params.stage.two_stage) {
        case TWO_STAGE_ONE: {
          if (sub_amt == 0) {
            tc_error err = {
              .type = AMBIGUOUS_TYPE,
              .pos = params.node_ind,
            };
            push_tc_err(state, err);
            break;
          }

          {
            NODE_IND_T sub_ind = PT_LIST_SUB_IND(inds, params.node, 0);
            tc_action action = {
              .tag = TC_NODE_UNAMBIGUOUS,
              .node_ind = sub_ind,
              .stage = {.two_stage = TWO_STAGE_ONE},
            };
            push_action(state, action);
          }

          for (NODE_IND_T i = 1; i < sub_amt; i++) {
            NODE_IND_T prev_sub_ind = PT_LIST_SUB_IND(inds, params.node, i);
            NODE_IND_T sub_ind = PT_LIST_SUB_IND(inds, params.node, i);
            tc_action actions[] = {{.tag = TC_CLONE_ACTUAL_WANTED,
                                    .from = prev_sub_ind,
                                    .to = sub_ind},
                                   {
                                     .tag = TC_NODE_UNAMBIGUOUS,
                                     .node_ind = sub_ind,
                                     .stage = {.two_stage = TWO_STAGE_ONE},
                                   }};
            push_actions(state, STATIC_LEN(actions), actions);
          }
          break;
        }
        case TWO_STAGE_TWO: {
          NODE_IND_T sub_ind = PT_LIST_SUB_IND(inds, params.node, 0);
          NODE_IND_T sub_type_ind = state->res.node_types[sub_ind];
          if (sub_amt == 0 || sub_type_ind == state->unknown_ind)
            break;
          state->res.node_types[params.node_ind] =
            mk_type_inline(state, T_LIST, sub_type_ind, 0);
        } break;
      }
      break;
    }

    case PT_AS: {
      tc_as(state, params);
      break;
    }

    case PT_TUP: {
      NODE_IND_T sub_amt = PT_TUP_SUB_AMT(params.node);
      switch (params.stage.two_stage) {
        case TWO_STAGE_ONE:
          for (NODE_IND_T i = 0; i < sub_amt; i++) {
            NODE_IND_T sub_ind =
              PT_TUP_SUB_IND(state->res.tree.inds, params.node, i);
            tc_action action = {
              .tag = TC_NODE_UNAMBIGUOUS,
              .node_ind = sub_ind,
              .stage = {.two_stage = TWO_STAGE_ONE},
            };
            push_action(state, action);
          }
          tc_action action = {
            .tag = TC_NODE_UNAMBIGUOUS,
            .node_ind = params.node_ind,
            .stage = {.two_stage = TWO_STAGE_TWO},
          };
          push_action(state, action);
          break;
        case TWO_STAGE_TWO: {
          size_t subs_bytes = sizeof(NODE_IND_T) * sub_amt;
          NODE_IND_T *subs = stalloc(subs_bytes);
          for (NODE_IND_T i = 0; i < sub_amt; i++) {
            NODE_IND_T sub_ind =
              PT_TUP_SUB_IND(state->res.tree.inds, params.node, i);
            subs[i] = state->res.node_types[sub_ind];
          }
          state->res.node_types[params.node_ind] =
            mk_type(state, T_TUP, subs, sub_amt);
          stfree(subs, subs_bytes);
          break;
        }
      }
      break;
    }

    case PT_IF: {
      NODE_IND_T cond = PT_IF_COND_IND(state->res.tree.inds, params.node);
      NODE_IND_T b1 = PT_IF_A_IND(state->res.tree.inds, params.node);
      NODE_IND_T b2 = PT_IF_B_IND(state->res.tree.inds, params.node);

      const tc_action actions[] = {
        {.tag = TC_WANT_TYPE, .from = state->bool_type_ind, .to = cond},
        {.tag = TC_NODE_MATCHES,
         .node_ind = cond,
         .stage = {.two_stage = TWO_STAGE_ONE}},
        {.tag = TC_NODE_UNAMBIGUOUS,
         .node_ind = b1,
         .stage = {.two_stage = TWO_STAGE_ONE}},
        {.tag = TC_CLONE_ACTUAL_WANTED, .from = b1, .to = b2},
        {.tag = TC_NODE_MATCHES,
         .node_ind = b2,
         .stage = {.two_stage = TWO_STAGE_ONE}},
        {.tag = TC_CLONE_ACTUAL_ACTUAL, .from = b2, .to = params.node_ind}};

      push_actions(state, STATIC_LEN(actions), actions);
      break;
    }

    case PT_INT: {
      tc_error err = {
        .type = AMBIGUOUS_TYPE,
        .pos = params.node_ind,
      };
      push_tc_err(state, err);
      break;
    }

    case PT_UPPER_NAME:
    case PT_LOWER_NAME: {
      binding b = node_to_binding(params.node);
      size_t ind = lookup_bnd(state->source.data, state->term_env,
                              state->term_is_builtin, b);
      if (ind == state->term_env.len) {
        tc_error err = {
          .type = BINDING_NOT_FOUND,
          .pos = params.node_ind,
        };
        push_tc_err(state, err);
        break;
      }
      NODE_IND_T type_ind = VEC_GET(state->term_type_inds, ind);
      tc_action action = {
        .tag = TC_ASSIGN_TYPE, .from = type_ind, .to = params.node_ind};
      push_action(state, action);
      break;
    }

    case PT_FN: {
      NODE_IND_T param_ind = PT_FN_PARAM_IND(params.node);
      NODE_IND_T body_ind = PT_FN_BODY_IND(params.node);

      switch (params.stage.two_stage) {
        case TWO_STAGE_ONE: {
          // TODO This is wrong. Need to add params to scope.
          tc_action actions[] = {
            {.tag = TC_NODE_UNAMBIGUOUS,
             .node_ind = param_ind,
             .stage = {.two_stage = TWO_STAGE_ONE}},
            {.tag = TC_NODE_UNAMBIGUOUS,
             .node_ind = body_ind,
             .stage = {.two_stage = TWO_STAGE_ONE}},
            {.tag = TC_POP_VARS_TO, .amt = state->term_env.len},
            {.tag = TC_NODE_UNAMBIGUOUS,
             .node_ind = params.node_ind,
             .stage = {.two_stage = TWO_STAGE_TWO}}};
          push_actions(state, STATIC_LEN(actions), actions);
          break;
        }
        case TWO_STAGE_TWO: {
          state->res.node_types[params.node_ind] =
            mk_type_inline(state, T_FN, state->res.node_types[param_ind],
                           state->res.node_types[body_ind]);
          break;
        }
      }
      break;
    }

    case PT_FUN: {
      NODE_IND_T bnd_ind =
        PT_FUN_BINDING_IND(state->res.tree.inds, params.node);
      NODE_IND_T param_ind =
        PT_FUN_PARAM_IND(state->res.tree.inds, params.node);
      NODE_IND_T body_ind = PT_FUN_BODY_IND(state->res.tree.inds, params.node);

      switch (params.stage.two_stage) {
        case TWO_STAGE_ONE: {
          tc_action actions[] = {
            {.tag = TC_NODE_UNAMBIGUOUS,
             .node_ind = param_ind,
             .stage = {.two_stage = TWO_STAGE_ONE}},
            {.tag = TC_NODE_UNAMBIGUOUS,
             .node_ind = body_ind,
             .stage = {.two_stage = TWO_STAGE_ONE}},
            {.tag = TC_POP_VARS_TO, .amt = state->term_env.len},
            {.tag = TC_NODE_UNAMBIGUOUS,
             .node_ind = params.node_ind,
             .stage = {.two_stage = TWO_STAGE_TWO}}};
          push_actions(state, STATIC_LEN(actions), actions);
          break;
        }
        case TWO_STAGE_TWO: {
          // TODO consider using wanted value?
          NODE_IND_T type =
            mk_type_inline(state, T_FN, state->res.node_types[param_ind],
                           state->res.node_types[body_ind]);
          state->res.node_types[params.node_ind] = type;
          state->res.node_types[bnd_ind] = type;
          break;
        }
      }
      break;
    }

    case PT_CALL:
      tc_call(state, params);
      break;

    case PT_LIST_TYPE:
    case PT_FN_TYPE:
      give_up("Can't typecheck term");
      break;
  }
}

static tc_node_params mk_tc_node_params(typecheck_state *state) {
  tc_node_params params;
  params.node_ind = state->current_node_ind;
  params.stage = state->current_stage;
  params.node = state->res.tree.nodes[params.node_ind];
  params.wanted_ind = state->wanted[params.node_ind];
  params.wanted = VEC_GET(state->res.types, params.wanted_ind);
  return params;
}

#ifdef DEBUG_TC
static char *const action_str(action_tag tag) {
  static const char *res;
  switch (tag) {
#define MK_CASE(e)                                                             \
  case (e):                                                                    \
    res = #e;                                                                  \
    break;
    MK_CASE(TC_POP_N_VARS)
    MK_CASE(TC_POP_VARS_TO)
    MK_CASE(TC_NODE_MATCHES)
    MK_CASE(TC_NODE_UNAMBIGUOUS)
    MK_CASE(TC_TYPE)
    MK_CASE(TC_CLONE_ACTUAL_ACTUAL)
    MK_CASE(TC_CLONE_ACTUAL_WANTED)
    MK_CASE(TC_CLONE_WANTED_WANTED)
    MK_CASE(TC_CLONE_WANTED_ACTUAL)
    MK_CASE(TC_WANT_TYPE)
    MK_CASE(TC_ASSIGN_TYPE)
    MK_CASE(TC_RECOVER)
#undef MK_CASE
  }
  return res;
};
#endif

static typecheck_state tc_new_state(source_file source, parse_tree tree) {
  typecheck_state state = {
    .res = {.types = VEC_NEW,
            .errors = VEC_NEW,
            .source = source,
            .node_types = malloc(tree.node_amt * sizeof(NODE_IND_T)),
            .tree = tree},
    .wanted = malloc(tree.node_amt * sizeof(NODE_IND_T)),

    .type_env = VEC_NEW,
    .type_is_builtin = bs_new(),
    .type_type_inds = VEC_NEW,

    .term_env = VEC_NEW,
    .term_is_builtin = bs_new(),
    .term_type_inds = VEC_NEW,

    .stack = VEC_NEW,

    .source = source,
  };
  tc_action action = {.tag = TC_NODE_UNAMBIGUOUS,
                      .node_ind = tree.root_ind,
                      .stage = {.two_stage = TWO_STAGE_ONE}};
  VEC_PUSH(&state.stack, action);
  return state;
}

static void print_tc_error(FILE *f, tc_res res, NODE_IND_T err_ind) {
  tc_error error = VEC_GET(res.errors, err_ind);
  switch (error.type) {
    case CALLED_NON_FUNCTION:
      fputs("Tried to call a non-function", f);
      break;
    case TUPLE_WRONG_ARITY:
      fputs("Wrong number of tuple elements", f);
      break;
    case NEED_SIGNATURE:
      fputs("Top level needs signature", f);
      break;
    case INT_LARGER_THAN_MAX:
      fputs("Int doesn't fit into type", f);
      break;
    case TYPE_NOT_FOUND:
      fputs("Type not in scope", f);
      break;
    case AMBIGUOUS_TYPE:
      fputs("Ambiguous type", f);
      break;
    case BINDING_NOT_FOUND:
      fputs("Binding not found", f);
      break;
    case TYPE_MISMATCH:
      fputs("Type mismatch. Expected ", f);
      print_type(f, res.types.data, res.type_inds.data, error.expected);
      fputs("\nGot ", f);
      print_type(f, VEC_DATA_PTR(&res.types), VEC_DATA_PTR(&res.type_inds),
                 error.got);
      break;
    case TYPE_HEAD_MISMATCH:
      fputs("Type mismatch. Expected ", f);
      print_type(f, res.types.data, res.type_inds.data, error.expected);
      fputs("\nGot: ", f);
      print_type_head_placeholders(f, error.got_type_head, error.got_arity);
      break;
    case LITERAL_MISMATCH:
      fputs("Literal mismatch. Expected ", f);
      print_type(f, VEC_DATA_PTR(&res.types), VEC_DATA_PTR(&res.type_inds),
                 error.expected);
      fputs("", f);
      break;
    case MISSING_SIG:
      fputs("Missing type signature", f);
      break;
    case WRONG_ARITY:
      fputs("Wrong arity", f);
      break;
  }
  parse_node node = res.tree.nodes[error.pos];
  fprintf(f, "\nAt %d-%d:\n", node.start, node.end);
  format_error_ctx(f, res.source.data, node.start, node.end);
}

void print_tc_errors(FILE *f, tc_res res) {
  for (size_t i = 0; i < res.errors.len; i++) {
    putc('\n', f);
    print_tc_error(f, res, i);
  }
}

tc_res typecheck(source_file source, parse_tree tree) {
  typecheck_state state = tc_new_state(source, tree);
  setup_type_env(&state);
  // resolve_types(&state);
#ifdef DEBUG_TC
  FILE *debug_out = fopen("debug-typechecker", "a");
  bool first_debug = true;
#endif

  while (state.stack.len > 0) {
    tc_action action = VEC_POP(&state.stack);
    size_t actions_start = state.stack.len;

#ifdef DEBUG_TC
    {
      if (!first_debug) {
        fputs("\n-----\n\n", debug_out);
      }
      first_debug = false;
      fprintf(debug_out, "Action: '%s'\n", action_str(action.tag));
      switch (action.tag) {
        case TC_POP_N_VARS:
          fprintf(debug_out, "Popping %d vars\n", action.amt);
          break;
        case TC_POP_VARS_TO:
          fprintf(debug_out, "Keeping %d vars\n", action.amt);
          break;
        case TC_WANT_TYPE: {
          parse_node node = tree.nodes[action.to];
          format_error_ctx(debug_out, source.data, node.start, node.end);
          putc('\n', debug_out);
          break;
        }
        case TC_CLONE_ACTUAL_ACTUAL:
        case TC_CLONE_ACTUAL_WANTED:
        case TC_CLONE_WANTED_ACTUAL:
        case TC_CLONE_WANTED_WANTED: {
          parse_node from_node = tree.nodes[action.from];
          fprintf(debug_out, "From: '%d' '%s'\n", action.from,
                  parse_node_string(from_node.type));
          format_error_ctx(debug_out, source.data, from_node.start,
                           from_node.end);
          putc('\n', debug_out);

          parse_node to_node = tree.nodes[action.to];
          fprintf(debug_out, "To: '%d' '%s'\n", action.to,
                  parse_node_string(to_node.type));
          format_error_ctx(debug_out, source.data, to_node.start, to_node.end);
          putc('\n', debug_out);
          break;
        }
        case TC_TYPE:
        case TC_NODE_MATCHES:
        case TC_NODE_UNAMBIGUOUS: {
          fprintf(debug_out, "Node ind: '%d'\n", action.node_ind);
          parse_node node = tree.nodes[action.node_ind];
          fprintf(debug_out, "Node: '%s'\n", parse_node_string(node.type));
          fprintf(debug_out, "Stage: '%d'\n", action.stage.four_stage);
          format_error_ctx(debug_out, source.data, node.start, node.end);
          putc('\n', debug_out);
          break;
        }
        default:
          fputs("No start debug info\n", debug_out);
          break;
      }
    }
    size_t starting_err_amt = state.res.errors.len;
#endif

    switch (action.tag) {
      case TC_CLONE_ACTUAL_ACTUAL:
        state.res.node_types[action.to] = state.res.node_types[action.from];
        break;
      case TC_CLONE_ACTUAL_WANTED:
        state.wanted[action.to] = state.res.node_types[action.from];
        break;
      case TC_CLONE_WANTED_ACTUAL:
        state.res.node_types[action.to] = state.wanted[action.from];
        break;
      case TC_CLONE_WANTED_WANTED:
        state.wanted[action.to] = state.wanted[action.from];
        break;
      case TC_POP_N_VARS:
        VEC_POP_N(&state.term_env.len, action.amt);
        break;
      case TC_POP_VARS_TO:
        VEC_POP_N(&state.term_env.len, state.term_env.len - action.amt);
        break;
      case TC_NODE_UNAMBIGUOUS:
        state.current_node_ind = action.node_ind;
        state.current_stage = action.stage;
        tc_node_unambiguous(&state, mk_tc_node_params(&state));
        break;
      case TC_NODE_MATCHES:
        state.current_node_ind = action.node_ind;
        state.current_stage = action.stage;
        tc_node_matches(&state, mk_tc_node_params(&state));
        break;
      case TC_TYPE:
        state.current_node_ind = action.node_ind;
        state.current_stage = action.stage;
        tc_type(&state);
        break;
      case TC_ASSIGN_TYPE:
        state.res.node_types[action.to] = action.from;
        break;
      case TC_WANT_TYPE:
        state.wanted[action.to] = action.from;
        break;
      case TC_RECOVER: {
        tc_action a = VEC_POP(&state.stack);
        tc_action b = VEC_POP(&state.stack);
        size_t err_diff = state.res.errors.len - action.amt;
        if (err_diff == 0) {
          VEC_PUSH(&state.stack, a);
        } else {
          VEC_POP_N(&state.res.errors, err_diff);
          VEC_PUSH(&state.stack, b);
        }
        break;
      }
    }

#ifdef DEBUG_TC
    if (state.res.errors.len > starting_err_amt) {
      fprintf(debug_out, "Caused an error.\n");
    }
    switch (action.tag) {
      case TC_CLONE_ACTUAL_WANTED:
      case TC_CLONE_WANTED_WANTED:
      case TC_WANT_TYPE:
        fputs("New wanted: ", debug_out);
        print_type(debug_out, state.res.types.data, state.res.type_inds.data,
                   state.wanted[action.to]);
        putc('\n', debug_out);
        break;
      case TC_TYPE:
      case TC_NODE_MATCHES:
      case TC_NODE_UNAMBIGUOUS:
      case TC_CLONE_ACTUAL_ACTUAL:
      case TC_CLONE_WANTED_ACTUAL:
      case TC_ASSIGN_TYPE:
        fputs("Result: ", debug_out);
        print_type(debug_out, state.res.types.data, state.res.type_inds.data,
                   state.res.node_types[action.node_ind]);
        putc('\n', debug_out);
        break;
      default:
        fputs("No end debug info\n", debug_out);
        break;
    }

#endif

    reverse_arbitrary(VEC_GET_PTR(state.stack, actions_start),
                      MAX(actions_start, state.stack.len) - actions_start,
                      sizeof(state.stack.data[0]));
  }

  VEC_FREE(&state.stack);

  VEC_FREE(&state.type_env);
  bs_free(&state.type_is_builtin);
  VEC_FREE(&state.type_type_inds);

  VEC_FREE(&state.term_env);
  bs_free(&state.term_is_builtin);
  VEC_FREE(&state.term_type_inds);
  free(state.wanted);

#ifdef DEBUG_TC
  fclose(debug_out);
#endif

  return state.res;
}

void free_tc_res(tc_res res) {
  VEC_FREE(&res.errors);
  VEC_FREE(&res.types);
  VEC_FREE(&res.type_inds);
  free(res.node_types);
}
