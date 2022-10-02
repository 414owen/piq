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
#include "types.h"
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


## Ideas

* Make action_tags for reconstructing types? Reconstruction seems to be the same
  everywhere.
* Separate constructor, type, and other bindings into their own arrays
  This would just be a shitty incomplete start of a trie, actually

*/

typedef enum {
  TC_POP_N_VARS,
  TC_POP_VARS_TO,

  TC_PUSH_ENV,

  // terms
  TC_NODE_MATCHES,
  TC_NODE_MATCHES_AS_STAGE_TWO,

  TC_NODE_UNAMBIGUOUS,
  TC_NODE_UNAMBIGUOUS_FN_STAGE_TWO,
  TC_NODE_UNAMBIGUOUS_FUN_STAGE_TWO,

  // types
  TC_TYPE,
  TC_TYPE_LIST_STAGE_TWO,
  TC_TYPE_FN_STAGE_TWO,
  TC_TYPE_CALL_STAGE_TWO,

  // Construct a compound type from its children
  TC_RECONSTRUCT_LIST,
  TC_RECONSTRUCT_TUPLE,

  TC_PATTERN_MATCHES,
  TC_PATTERN_UNAMBIGUOUS,

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

  TC_CALL_POST_PARAM,
  TC_CALL_POST_CALLEE,
  TC_CALL_PARAM_FIRST,
  TC_CALL_CALLEE_FIRST,

  // If there were errors on top of this node, pop them
  // and execute the action two below
  // Otherwise, execute the action one below
  TC_RECOVER,
} action_tag;

// This looks like ass.
typedef struct {
  action_tag tag;
  union {
    node_ind_t amt;
    struct {
      node_ind_t node_ind;
      binding binding;
    };
    struct {
      node_ind_t from;
      node_ind_t to;
    };
  };
} tc_action;

VEC_DECL(tc_action);

typedef struct {
  struct {
    // all the types
    vec_type types;
    // type sub-indices
    vec_node_ind type_inds;
    // index into types, one per parse_node
    node_ind_t *node_types;
  } types;

  vec_tc_error errors;

  // TODO technically it's more efficient to have a stack of node_ind_t,
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

  // types.node_types is wanted or known
  node_ind_t *wanted;
  node_ind_t unknown_ind;
  node_ind_t string_type_ind;
  node_ind_t bool_type_ind;

  parse_tree tree;
  const char *restrict input;
} typecheck_state;

typedef struct {
  node_ind_t node_ind;
  parse_node node;
  node_ind_t wanted_ind;
  type wanted;
} tc_node_params;

static void push_tc_err(typecheck_state *state, tc_error err) {
  VEC_PUSH(&state->errors, err);
}

static node_ind_t find_primitive_type(typecheck_state *state, type_tag tag) {
  for (node_ind_t i = 0; i < state->types.types.len; i++) {
    type t = VEC_GET(state->types.types, i);
    if (t.tag == tag)
      return i;
  }
  return state->types.types.len;
}

static node_ind_t find_type(typecheck_state *state, type_tag tag,
                            node_ind_t *subs, node_ind_t sub_amt) {
  for (node_ind_t i = 0; i < state->types.types.len; i++) {
    type t = VEC_GET(state->types.types, i);
    if (t.tag != tag || t.sub_amt != sub_amt)
      continue;
    node_ind_t *p1 = &VEC_DATA_PTR(&state->types.type_inds)[t.subs_start];
    if (memcmp(p1, subs, sub_amt) != 0)
      continue;
    return i;
  }
  return state->types.types.len;
}

static node_ind_t mk_primitive_type(typecheck_state *state, type_tag tag) {
  debug_assert(type_repr(tag) == SUBS_NONE);
  node_ind_t ind = find_primitive_type(state, tag);
  if (ind < state->types.types.len)
    return ind;
  type t = {
    .tag = tag,
    .sub_amt = 0,
    .subs_start = 0,
  };
  VEC_PUSH(&state->types.types, t);
  return state->types.types.len - 1;
}

static node_ind_t mk_type_inline(typecheck_state *state, type_tag tag,
                                 node_ind_t sub_a, node_ind_t sub_b) {
  debug_assert(type_repr(tag) == SUBS_ONE || type_repr(tag) == SUBS_TWO);
  type t = {
    .tag = tag,
    .sub_a = sub_a,
    .sub_b = sub_b,
  };
  for (size_t i = 0; i < state->types.types.len; i++) {
    type t2 = VEC_GET(state->types.types, i);
    if (inline_types_eq(t, t2)) {
      return i;
    }
  }
  VEC_PUSH(&state->types.types, t);
  return state->types.types.len - 1;
}

static node_ind_t mk_type(typecheck_state *state, type_tag tag,
                          node_ind_t *subs, node_ind_t sub_amt) {
  debug_assert(type_repr(tag) == SUBS_EXTERNAL);
  if (subs == NULL) {
    return mk_primitive_type(state, tag);
  }
  node_ind_t ind = find_type(state, tag, subs, sub_amt);
  if (ind < state->types.types.len)
    return ind;
  debug_assert(sizeof(subs[0]) == sizeof(VEC_GET(state->types.type_inds, 0)));
  // size_t find_range(const void *haystack, size_t el_size, size_t el_amt,
  // const void *needle, size_t needle_els);
  ind = find_range(VEC_DATA_PTR(&state->types.type_inds), sizeof(subs[0]),
                   state->types.type_inds.len, subs, sub_amt);
  if (ind == state->types.type_inds.len) {
    VEC_APPEND(&state->types.type_inds, sub_amt, subs);
  }
  type t = {
    .tag = tag,
    .sub_amt = sub_amt,
    .subs_start = ind,
  };
  VEC_PUSH(&state->types.types, t);
  return state->types.types.len - 1;
}

#ifdef DEBUG_TC
static void suspicious_action() {}

static void break_suspicious_action(typecheck_state *state, tc_action action) {
  switch (action.tag) {
    case TC_POP_N_VARS:
      if (action.amt > 100)
        suspicious_action();
      break;
    case TC_POP_VARS_TO:
      if (action.amt > 10000)
        suspicious_action();
      break;
    case TC_PATTERN_MATCHES:
    case TC_PATTERN_UNAMBIGUOUS:
    case TC_NODE_MATCHES:
    case TC_NODE_MATCHES_AS_STAGE_TWO:
    case TC_NODE_UNAMBIGUOUS:
    case TC_NODE_UNAMBIGUOUS_FN_STAGE_TWO:
    case TC_NODE_UNAMBIGUOUS_FUN_STAGE_TWO:
    case TC_TYPE:
    case TC_TYPE_LIST_STAGE_TWO:
    case TC_TYPE_FN_STAGE_TWO:
    case TC_TYPE_CALL_STAGE_TWO:
    case TC_CALL_POST_PARAM:
    case TC_CALL_POST_CALLEE:
    case TC_CALL_PARAM_FIRST:
    case TC_CALL_CALLEE_FIRST:
    case TC_RECONSTRUCT_LIST:
    case TC_RECONSTRUCT_TUPLE:
      if (action.node_ind >= state->tree.node_amt)
        suspicious_action();
      break;
    case TC_CLONE_ACTUAL_ACTUAL:
    case TC_CLONE_ACTUAL_WANTED:
    case TC_CLONE_WANTED_ACTUAL:
      if (action.from == 0 && action.to == 0)
        suspicious_action();
      HEDLEY_FALL_THROUGH;
    case TC_CLONE_WANTED_WANTED:
      if (action.from >= state->tree.node_amt)
        suspicious_action();
      if (action.to >= state->tree.node_amt)
        suspicious_action();
      break;
    case TC_ASSIGN_TYPE:
    case TC_WANT_TYPE:
      if (action.to >= state->tree.node_amt)
        suspicious_action();
      if (action.from >= state->types.types.len)
        suspicious_action();
      break;
    case TC_RECOVER:
      if (state->stack.len < 2)
        suspicious_action();
      break;
    case TC_PUSH_ENV:
      break;
  }
}
#endif

// This is for instrumentation purposes only
static void push_action(typecheck_state *state, tc_action act) {
#ifdef DEBUG_TC
  break_suspicious_action(state, act);
#endif
  VEC_PUSH(&state->stack, act);
}

static void push_actions(typecheck_state *state, node_ind_t amt,
                         const tc_action *actions) {
#ifdef DEBUG_TC
  for (node_ind_t i = 0; i < amt; i++) {
    tc_action action = actions[i];
    break_suspicious_action(state, action);
  }
#endif
  VEC_APPEND(&state->stack, amt, actions);
}

// Add builtin types and custom types to type env
static void setup_type_env(typecheck_state *state) {

  {
    type t = {.tag = T_UNKNOWN, .sub_amt = 0, .subs_start = 0};
    VEC_PUSH(&state->types.types, t);
    state->unknown_ind = state->types.types.len - 1;
  }

  memset_arbitrary(state->types.node_types, &state->unknown_ind,
                   state->tree.node_amt, sizeof(node_ind_t));
  memset_arbitrary(state->wanted, &state->unknown_ind, state->tree.node_amt,
                   sizeof(node_ind_t));

  // Prelude
  {
    static const char *builtin_names[] = {
      "Bool", "U8", "U16", "U32", "U64", "I8", "I16", "I32", "I64", "String"};

    static const type_tag primitive_types[] = {
      T_BOOL, T_U8, T_U16, T_U32, T_U64, T_I8, T_I16, T_I32, T_I64};

    VEC_APPEND(&state->type_env, STATIC_LEN(builtin_names), builtin_names);
    node_ind_t start_type_ind = state->types.types.len;
    for (node_ind_t i = 0; i < STATIC_LEN(primitive_types); i++) {
      VEC_PUSH(&state->type_type_inds, start_type_ind + i);
      type t = {.tag = primitive_types[i], .subs_start = 0, .sub_amt = 0};
      VEC_PUSH(&state->types.types, t);
      bs_push(&state->type_is_builtin, true);
    }

    {
      node_ind_t u8_ind = mk_primitive_type(state, T_U8);
      node_ind_t list_ind = mk_type_inline(state, T_LIST, u8_ind, 0);
      state->string_type_ind = list_ind;
      VEC_PUSH(&state->type_type_inds, state->types.types.len - 1);
      bs_push(&state->type_is_builtin, true);
    }
    state->bool_type_ind = mk_primitive_type(state, T_BOOL);
  }

  // Declared here
  {
    for (size_t i = 0; i < state->tree.node_amt; i++) {
      parse_node node = state->tree.nodes[i];
      switch (node.type) {
        // TODO fill in user declared types here
        default:
          break;
      }
    }
  }
}

static bool check_int_fits(typecheck_state *state, node_ind_t node_ind,
                           uint64_t max) {
  parse_node node = state->tree.nodes[node_ind];
  bool res = true;
  const char *buf = state->input;
  uint64_t last, n = 0;
  for (size_t i = node.span.start; i <= node.span.end; i++) {
    last = n;
    char digit = buf[i];
    n *= 10;
    n += digit - '0';
    if (n < last || n > max) {
      tc_error err = {
        .type = INT_LARGER_THAN_MAX,
        .pos = node_ind,
      };
      push_tc_err(state, err);
      res = false;
      break;
    }
  }
  return res;
}

static bool check_int_fits_type(typecheck_state *state, node_ind_t node_ind,
                                node_ind_t wanted_ind) {
  type wanted = VEC_GET(state->types.types, wanted_ind);
  bool fits = true;
  switch (wanted.tag) {
    case T_U8:
      fits = check_int_fits(state, node_ind, UINT8_MAX);
      break;
    case T_U16:
      fits = check_int_fits(state, node_ind, UINT16_MAX);
      break;
    case T_U32:
      fits = check_int_fits(state, node_ind, UINT32_MAX);
      break;
    case T_U64:
      fits = check_int_fits(state, node_ind, UINT64_MAX);
      break;
    case T_I8:
      fits = check_int_fits(state, node_ind, INT8_MAX);
      break;
    case T_I16:
      fits = check_int_fits(state, node_ind, INT16_MAX);
      break;
    case T_I32:
      fits = check_int_fits(state, node_ind, INT32_MAX);
      break;
    case T_I64:
      fits = check_int_fits(state, node_ind, INT64_MAX);
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

static bool can_propagate_type(typecheck_state *state, parse_node from,
                               parse_node to) {
  node_ind_t a_bnd_ind;
  node_ind_t b_bnd_ind;
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
      a_bnd_ind = PT_FUN_BINDING_IND(state->tree.inds, to);
      switch (from.type) {
        case PT_SIG:
          b_bnd_ind = PT_SIG_BINDING_IND(from);
          break;
        case PT_FUN:
          b_bnd_ind = PT_FUN_BINDING_IND(state->tree.inds, from);
          break;
        default:
          return false;
      }
      break;
    default:
      return false;
  }
  return compare_bnds(state->input, state->tree.nodes[a_bnd_ind].span,
                      state->tree.nodes[b_bnd_ind].span) == EQ;
}

static void push_tc_sig(typecheck_state *state, node_ind_t node_ind,
                        parse_node node) {
  node_ind_t name_ind = PT_SIG_BINDING_IND(node);
  node_ind_t type_ind = PT_SIG_TYPE_IND(node);
  tc_action actions[] = {
    {
      .tag = TC_TYPE,
      .node_ind = type_ind,
    },
    {.tag = TC_CLONE_ACTUAL_ACTUAL, .from = type_ind, .to = name_ind},
    {.tag = TC_CLONE_ACTUAL_ACTUAL, .from = type_ind, .to = node_ind},
    {.tag = TC_PUSH_ENV,
     .node_ind = type_ind,
     .binding = state->tree.nodes[name_ind].span},
  };
  push_actions(state, STATIC_LEN(actions), actions);
}

// enforce sigs => we're root level
static void typecheck_block(typecheck_state *state, tc_node_params params,
                            bool enforce_sigs) {
  bool can_continue = true;

  if (params.node.sub_amt == 0)
    return;

  char *sub_has_wanted = stcalloc(BITNSLOTS(params.node.sub_amt), 1);

  node_ind_t last_el_ind =
    state->tree.inds[params.node.subs_start + params.node.sub_amt - 1];

  size_t start_action_amt = state->stack.len;

  if (params.wanted_ind != state->unknown_ind) {
    tc_action action = {.tag = TC_CLONE_WANTED_WANTED,
                        .from = params.node_ind,
                        .to = last_el_ind};
    push_action(state, action);
    bs_data_set(sub_has_wanted, params.node.sub_amt - 1, true);
  }

  if (params.node.sub_amt == 0)
    return;

  {
    node_ind_t sub_ind = state->tree.inds[params.node.subs_start];
    parse_node sub = state->tree.nodes[sub_ind];
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
  for (node_ind_t sub_i = 1; sub_i < params.node.sub_amt; sub_i++) {
    node_ind_t sub_ind = state->tree.inds[params.node.subs_start + sub_i];
    parse_node sub = state->tree.nodes[sub_ind];
    switch (sub.type) {
      case PT_SIG: {
        push_tc_sig(state, sub_ind, sub);
        break;
      }
      case PT_LET:
      case PT_FUN: {
        node_ind_t prev_ind =
          state->tree.inds[params.node.subs_start + sub_i - 1];
        parse_node prev = state->tree.nodes[prev_ind];
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
    stfree(sub_has_wanted, BITNSLOTS(params.node.sub_amt));
    return;
  }

  node_ind_t bnd_amt = 0;
  for (node_ind_t i = 0; i < params.node.sub_amt; i++) {
    node_ind_t sub_ind = state->tree.inds[params.node.subs_start + i];
    parse_node sub = state->tree.nodes[sub_ind];
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
        tc_action action = {
          .tag = tag,
          .node_ind = sub_ind,
        };
        push_action(state, action);
        break;
      }
    }
  }
  tc_action actions[] = {{.tag = TC_POP_N_VARS, .amt = bnd_amt},
                         {.tag = TC_CLONE_ACTUAL_ACTUAL,
                          .from = last_el_ind,
                          .to = params.node_ind}};
  push_actions(state, STATIC_LEN(actions), actions);

  stfree(sub_has_wanted, BITNSLOTS(params.node.sub_amt));
}

static void tc_as(typecheck_state *state, tc_node_params params) {
  node_ind_t val_node_ind = PT_AS_VAL_IND(params.node);
  node_ind_t type_node_ind = PT_AS_TYPE_IND(params.node);
  tc_action actions[] = {
    {
      .tag = TC_TYPE,
      .node_ind = type_node_ind,
    },
    {.tag = TC_CLONE_ACTUAL_WANTED, .from = type_node_ind, .to = val_node_ind},
    {.tag = TC_CLONE_ACTUAL_ACTUAL,
     .from = type_node_ind,
     .to = params.node_ind},
    {
      .tag = TC_NODE_MATCHES,
      .node_ind = val_node_ind,
    },
  };
  push_actions(state, STATIC_LEN(actions), actions);
}

static void tc_call_param_first(typecheck_state *state,
                                node_ind_t call_node_ind) {
  parse_node call_node = state->tree.nodes[call_node_ind];
  node_ind_t param_ind = PT_CALL_PARAM_IND(call_node);
  tc_action actions[] = {{
                           .tag = TC_NODE_UNAMBIGUOUS,
                           .node_ind = param_ind,
                         },
                         {
                           .tag = TC_CALL_POST_PARAM,
                           .node_ind = call_node_ind,
                         }};
  push_actions(state, STATIC_LEN(actions), actions);
}

static void tc_call_callee_first(typecheck_state *state,
                                 node_ind_t call_node_ind) {
  parse_node call_node = state->tree.nodes[call_node_ind];
  node_ind_t callee_ind = PT_CALL_CALLEE_IND(call_node);

  // TODO make it a matches call to a fn type
  tc_action actions[] = {{
                           .tag = TC_NODE_UNAMBIGUOUS,
                           .node_ind = callee_ind,
                         },
                         {
                           .tag = TC_CALL_POST_CALLEE,
                           .node_ind = call_node_ind,
                         }};
  push_actions(state, STATIC_LEN(actions), actions);
}

static void tc_call(typecheck_state *state, tc_node_params params) {
  node_ind_t callee_ind = PT_CALL_CALLEE_IND(params.node);
  parse_node callee = state->tree.nodes[callee_ind];

  switch (callee.type) {
    case PT_FN:
      tc_call_param_first(state, params.node_ind);
      break;

    case PT_UPPER_NAME:
      tc_call_callee_first(state, params.node_ind);
      break;

    default: {
      tc_action actions[] = {{.tag = TC_RECOVER, .amt = state->errors.len},
                             {
                               .tag = TC_CALL_PARAM_FIRST,
                               .node_ind = params.node_ind,
                             },
                             {
                               .tag = TC_CALL_CALLEE_FIRST,
                               .node_ind = params.node_ind,
                             }};
      push_actions(state, STATIC_LEN(actions), actions);
      break;
    }
  }
}

static void push_list_subs_unambiguous(typecheck_state *state,
                                       tc_node_params params,
                                       action_tag first_tag,
                                       action_tag wanted_tag) {
  if (PT_LIST_SUB_AMT(params.node) == 0) {
    tc_error err = {
      .type = AMBIGUOUS_TYPE,
      .pos = params.node_ind,
    };
    push_tc_err(state, err);
    return;
  }

  node_ind_t *inds = state->tree.inds;

  {
    node_ind_t sub_ind = PT_LIST_SUB_IND(inds, params.node, 0);
    tc_action action = {
      .tag = first_tag,
      .node_ind = sub_ind,
    };
    push_action(state, action);
  }

  for (node_ind_t i = 1; i < PT_LIST_SUB_AMT(params.node); i++) {
    node_ind_t prev_sub_ind = PT_LIST_SUB_IND(inds, params.node, i);
    node_ind_t sub_ind = PT_LIST_SUB_IND(inds, params.node, i);
    tc_action actions[] = {
      {.tag = TC_CLONE_ACTUAL_WANTED, .from = prev_sub_ind, .to = sub_ind},
      {
        .tag = wanted_tag,
        .node_ind = sub_ind,
      }};
    push_actions(state, STATIC_LEN(actions), actions);
  }

  tc_action action = {.tag = TC_RECONSTRUCT_LIST, .node_ind = params.node_ind};
  push_action(state, action);
}

static void push_list_subs_match(typecheck_state *state, tc_node_params params,
                                 action_tag tag) {
  if (params.wanted.tag != T_LIST) {
    tc_error err = {
      .type = LITERAL_MISMATCH,
      .pos = params.node_ind,
      .expected = params.wanted_ind,
    };
    push_tc_err(state, err);
    return;
  }
  node_ind_t wanted_sub_ind = T_LIST_SUB_IND(params.wanted);
  node_ind_t sub_amt = PT_LIST_SUB_AMT(params.node);

  // Why push them in order? I figure smaller loops and reppeatedly running the
  // same code is worth the duplication?
  for (node_ind_t i = 0; i < sub_amt; i++) {
    node_ind_t sub_ind = PT_LIST_SUB_IND(state->tree.inds, params.node, i);
    tc_action action = {
      .tag = TC_WANT_TYPE, .from = wanted_sub_ind, .to = sub_ind};
    push_action(state, action);
  }
  for (node_ind_t i = 0; i < sub_amt; i++) {
    node_ind_t sub_ind = PT_LIST_SUB_IND(state->tree.inds, params.node, i);
    tc_action action = {
      .tag = tag,
      .node_ind = sub_ind,
    };
    push_action(state, action);
  }
}

static void push_tuple_subs(typecheck_state *state, tc_node_params params,
                            action_tag tag) {
  tc_action actions[2] = {{
                            .tag = tag,
                            .node_ind = PT_TUP_SUB_A(params.node),
                          },
                          {
                            .tag = tag,
                            .node_ind = PT_TUP_SUB_B(params.node),
                          }};
  push_actions(state, STATIC_LEN(actions), actions);
}

static void tc_pattern_matches(typecheck_state *state, tc_node_params params) {
  switch (params.node.type) {
    case PT_UNIT: {
      if (params.wanted.tag != T_UNIT) {
        tc_error err = {
          .type = LITERAL_MISMATCH,
          .pos = params.node_ind,
          .expected = params.wanted_ind,
        };
        push_tc_err(state, err);
      }
      break;
    }
    case PT_CONSTRUCTION: {
      // TODO
      UNIMPLEMENTED("Typechecking constructors");
      break;
    }
    case PT_STRING: {
      if (params.wanted.tag != T_UNIT) {
        tc_error err = {
          .type = LITERAL_MISMATCH,
          .pos = params.node_ind,
          .expected = params.wanted_ind,
        };
        push_tc_err(state, err);
      }
      break;
    }

    case PT_LIST: {
      push_list_subs_match(state, params, TC_PATTERN_MATCHES);
      break;
    }
    case PT_TUP: {
      push_tuple_subs(state, params, TC_PATTERN_MATCHES);
      break;
    }

    case PT_UPPER_NAME: {
      binding b = params.node.span;
      size_t ind = lookup_str_ref(state->input, state->term_env,
                                  state->term_is_builtin, b);
      if (ind == state->term_env.len) {
        tc_error err = {
          .type = BINDING_NOT_FOUND,
          .pos = params.node_ind,
        };
        push_tc_err(state, err);
        break;
      }
      node_ind_t type_ind = VEC_GET(state->term_type_inds, ind);
      tc_action action = {
        .tag = TC_ASSIGN_TYPE, .from = type_ind, .to = params.node_ind};
      push_action(state, action);
      break;
    }

    case PT_LOWER_NAME: {
      tc_action action = {
        .tag = TC_PUSH_ENV,
        .binding = params.node.span,
        .node_ind = params.node_ind,
      };
      push_action(state, action);
      break;
    }
    case PT_INT: {
      check_int_fits_type(state, params.node_ind, params.wanted_ind);
      break;
    }

    case PT_FUN_BODY:
    case PT_ROOT:
    case PT_IF:
    case PT_FN:
    case PT_FUN:
    case PT_LIST_TYPE:
    case PT_FN_TYPE:
    case PT_CALL:
    case PT_AS:
    case PT_SIG:
    case PT_LET: {
      give_up("Unexpected pattern construct");
      break;
    }
  }
}

static void tc_pattern_unambiguous(typecheck_state *state,
                                   tc_node_params params) {
  switch (params.node.type) {
    case PT_UNIT: {
      state->types.node_types[params.node_ind] =
        mk_primitive_type(state, T_UNIT);
      break;
    }
    case PT_CONSTRUCTION: {
      // TODO
      UNIMPLEMENTED("Typechecking constructors");
      break;
    }
    case PT_STRING: {
      state->types.node_types[params.node_ind] = state->string_type_ind;
      break;
    }
    case PT_LIST: {
      push_list_subs_unambiguous(state, params, TC_PATTERN_MATCHES,
                                 TC_PATTERN_MATCHES);
      // Screw it, reconstruction is the same for nodes. Let's make them do it.
      tc_action action = {
        .tag = TC_NODE_MATCHES,
        .node_ind = params.node_ind,
      };
      push_action(state, action);
      break;
    }

    case PT_TUP: {
      // TODO extract pushing out, as with push_list_subs()
      push_tuple_subs(state, params, TC_PATTERN_UNAMBIGUOUS);
      tc_action action = {.tag = TC_RECONSTRUCT_TUPLE,
                          .node_ind = params.node_ind};
      push_action(state, action);
      break;
    }

    // TODO This will become valid with generics
    case PT_LOWER_NAME:
    case PT_INT: {
      tc_error err = {
        .type = AMBIGUOUS_TYPE,
        .pos = params.node_ind,
      };
      push_tc_err(state, err);
      break;
    }

    case PT_UPPER_NAME: {
      binding b = params.node.span;
      size_t ind = lookup_str_ref(state->input, state->term_env,
                                  state->term_is_builtin, b);
      if (ind == state->term_env.len) {
        tc_error err = {
          .type = BINDING_NOT_FOUND,
          .pos = params.node_ind,
        };
        push_tc_err(state, err);
        break;
      }
      node_ind_t type_ind = VEC_GET(state->term_type_inds, ind);
      tc_action action = {
        .tag = TC_ASSIGN_TYPE, .from = type_ind, .to = params.node_ind};
      push_action(state, action);
      break;
    }

    case PT_FUN_BODY:
    case PT_ROOT:
    case PT_IF:
    case PT_FN:
    case PT_AS:
    case PT_FUN:
    case PT_LIST_TYPE:
    case PT_FN_TYPE:
    case PT_CALL:
    case PT_SIG:
    case PT_LET: {
      give_up("Unexpected pattern construct");
      break;
    }
  }
}

static tc_node_params mk_tc_node_params(typecheck_state *state,
                                        node_ind_t node_ind) {
  tc_node_params params;
  params.node_ind = node_ind;
  params.node = state->tree.nodes[params.node_ind];
  params.wanted_ind = state->wanted[params.node_ind];
  params.wanted = VEC_GET(state->types.types, params.wanted_ind);
  return params;
}

#ifdef DEBUG_TC
static const char *action_str(action_tag tag) {
  static const char *res;
  switch (tag) {
#define MK_CASE(e)                                                             \
  case (e):                                                                    \
    res = #e;                                                                  \
    break;
    MK_CASE(TC_POP_N_VARS)
    MK_CASE(TC_POP_VARS_TO)
    MK_CASE(TC_NODE_MATCHES)
    MK_CASE(TC_NODE_MATCHES_AS_STAGE_TWO)
    MK_CASE(TC_NODE_UNAMBIGUOUS)
    MK_CASE(TC_NODE_UNAMBIGUOUS_FN_STAGE_TWO)
    MK_CASE(TC_NODE_UNAMBIGUOUS_FUN_STAGE_TWO)
    MK_CASE(TC_PATTERN_MATCHES)
    MK_CASE(TC_PATTERN_UNAMBIGUOUS)
    MK_CASE(TC_TYPE)
    MK_CASE(TC_TYPE_LIST_STAGE_TWO)
    MK_CASE(TC_TYPE_CALL_STAGE_TWO)
    MK_CASE(TC_TYPE_FN_STAGE_TWO)
    MK_CASE(TC_CLONE_ACTUAL_ACTUAL)
    MK_CASE(TC_CLONE_ACTUAL_WANTED)
    MK_CASE(TC_CLONE_WANTED_WANTED)
    MK_CASE(TC_CLONE_WANTED_ACTUAL)
    MK_CASE(TC_WANT_TYPE)
    MK_CASE(TC_ASSIGN_TYPE)
    MK_CASE(TC_RECOVER)
    MK_CASE(TC_PUSH_ENV)
    MK_CASE(TC_RECONSTRUCT_LIST)
    MK_CASE(TC_RECONSTRUCT_TUPLE)
    MK_CASE(TC_CALL_POST_PARAM)
    MK_CASE(TC_CALL_POST_CALLEE)
    MK_CASE(TC_CALL_PARAM_FIRST)
    MK_CASE(TC_CALL_CALLEE_FIRST)
#undef MK_CASE
  }
  return res;
};
#endif

static typecheck_state tc_new_state(const char *restrict input,
                                    parse_tree tree) {
  typecheck_state state = {
    .types = {.types = VEC_NEW,
              .type_inds = VEC_NEW,
              .node_types = malloc(tree.node_amt * sizeof(node_ind_t))},
    .errors = VEC_NEW,
    .wanted = malloc(tree.node_amt * sizeof(node_ind_t)),

    .type_env = VEC_NEW,
    .type_is_builtin = bs_new(),
    .type_type_inds = VEC_NEW,

    .term_env = VEC_NEW,
    .term_is_builtin = bs_new(),
    .term_type_inds = VEC_NEW,

    .stack = VEC_NEW,

    .input = input,
    .tree = tree,
  };
  tc_action action = {
    .tag = TC_NODE_UNAMBIGUOUS,
    .node_ind = tree.root_ind,
  };
  VEC_PUSH(&state.stack, action);
  return state;
}

static void print_tc_error(FILE *f, tc_res res, const char *restrict input,
                           parse_tree tree, node_ind_t err_ind) {
  tc_error error = res.errors[err_ind];
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
      print_type(f, res.types.types, error.expected);
      fputs("\nGot ", f);
      print_type(f, res.types.types, error.got);
      break;
    case TYPE_HEAD_MISMATCH:
      fputs("Type mismatch. Expected ", f);
      print_type(f, res.types.types, error.expected);
      fputs("\nGot: ", f);
      print_type_head_placeholders(f, error.got_type_head);
      break;
    case LITERAL_MISMATCH:
      fputs("Literal mismatch. Expected ", f);
      print_type(f, res.types.types, error.expected);
      fputs("", f);
      break;
    case MISSING_SIG:
      fputs("Missing type signature", f);
      break;
    case WRONG_ARITY:
      fputs("Wrong arity", f);
      break;
  }
  parse_node node = tree.nodes[error.pos];
  fprintf(f, "\nAt %d-%d:\n", node.span.start, node.span.end);
  format_error_ctx(f, input, node.span.start, node.span.end);
}

void print_tc_errors(FILE *f, const char *restrict input, parse_tree tree,
                     tc_res res) {
  for (size_t i = 0; i < res.error_amt; i++) {
    putc('\n', f);
    print_tc_error(f, res, input, tree, i);
  }
}

tc_res typecheck(const char *restrict input, parse_tree tree) {
  typecheck_state state = tc_new_state(input, tree);
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
          format_error_ctx(debug_out, input, node.span.start, node.span.end);
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
          format_error_ctx(debug_out, input, from_node.span.start,
                           from_node.span.end);
          putc('\n', debug_out);

          parse_node to_node = tree.nodes[action.to];
          fprintf(debug_out, "To: '%d' '%s'\n", action.to,
                  parse_node_string(to_node.type));
          format_error_ctx(debug_out, input, to_node.span.start,
                           to_node.span.end);
          putc('\n', debug_out);
          break;
        }
        case TC_TYPE:
        case TC_TYPE_LIST_STAGE_TWO:
        case TC_RECONSTRUCT_LIST:
        case TC_RECONSTRUCT_TUPLE:
        case TC_NODE_MATCHES:
        case TC_NODE_MATCHES_AS_STAGE_TWO:
        case TC_PATTERN_UNAMBIGUOUS:
        case TC_PATTERN_MATCHES:
        case TC_NODE_UNAMBIGUOUS_FN_STAGE_TWO:
        case TC_NODE_UNAMBIGUOUS_FUN_STAGE_TWO:
        case TC_NODE_UNAMBIGUOUS: {
          fprintf(debug_out, "Node ind: '%d'\n", action.node_ind);
          parse_node node = tree.nodes[action.node_ind];
          fprintf(debug_out, "Node: '%s'\n", parse_node_string(node.type));
          format_error_ctx(debug_out, input, node.span.start, node.span.end);
          putc('\n', debug_out);
          break;
        }
        default:
          fputs("No start debug info\n", debug_out);
          break;
      }
    }
    size_t starting_err_amt = state.errors.len;
#endif
    tc_node_params node_params;

    switch (action.tag) {
      case TC_PATTERN_MATCHES:
      case TC_NODE_MATCHES_AS_STAGE_TWO:
      case TC_NODE_MATCHES: {
        node_ind_t node_ind = action.node_ind;
        tc_action a = {
          .tag = TC_CLONE_WANTED_ACTUAL, .from = node_ind, .to = node_ind};
        push_action(&state, a);
      }
        HEDLEY_FALL_THROUGH;
      case TC_PATTERN_UNAMBIGUOUS:
      case TC_TYPE:
      case TC_TYPE_LIST_STAGE_TWO:
      case TC_TYPE_FN_STAGE_TWO:
      case TC_TYPE_CALL_STAGE_TWO:
      case TC_NODE_UNAMBIGUOUS:
      case TC_NODE_UNAMBIGUOUS_FN_STAGE_TWO:
      case TC_NODE_UNAMBIGUOUS_FUN_STAGE_TWO:
      case TC_RECONSTRUCT_LIST:
      // TODO do these belong here?
      case TC_CALL_POST_PARAM:
      case TC_CALL_POST_CALLEE:
      case TC_CALL_PARAM_FIRST:
      case TC_CALL_CALLEE_FIRST:

      case TC_RECONSTRUCT_TUPLE:
        node_params = mk_tc_node_params(&state, action.node_ind);
        break;
      case TC_POP_N_VARS:
      case TC_POP_VARS_TO:
      case TC_PUSH_ENV:
      case TC_CLONE_ACTUAL_ACTUAL:
      case TC_CLONE_ACTUAL_WANTED:
      case TC_CLONE_WANTED_ACTUAL:
      case TC_CLONE_WANTED_WANTED:
      case TC_WANT_TYPE:
      case TC_ASSIGN_TYPE:
      case TC_RECOVER:
        break;
    }

    switch (action.tag) {

      case TC_CLONE_ACTUAL_ACTUAL:
        state.types.node_types[action.to] = state.types.node_types[action.from];
        break;

      case TC_CLONE_ACTUAL_WANTED:
        state.wanted[action.to] = state.types.node_types[action.from];
        break;

      case TC_CLONE_WANTED_ACTUAL:
        state.types.node_types[action.to] = state.wanted[action.from];
        break;

      case TC_PUSH_ENV:
        VEC_PUSH(&state.term_env, action.binding);
        bs_push(&state.term_is_builtin, false);
        VEC_PUSH(&state.term_type_inds,
                 state.types.node_types[action.node_ind]);
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

      case TC_CALL_PARAM_FIRST:
        tc_call_param_first(&state, action.node_ind);
        break;

      case TC_CALL_CALLEE_FIRST:
        tc_call_param_first(&state, action.node_ind);
        break;

      case TC_CALL_POST_PARAM: {
        node_ind_t callee_ind = PT_CALL_CALLEE_IND(node_params.node);
        node_ind_t param_ind = PT_CALL_PARAM_IND(node_params.node);
        node_ind_t wanted_fn_type =
          mk_type_inline(&state, T_FN, state.types.node_types[param_ind],
                         node_params.wanted_ind);
        tc_action actions[] = {
          {.tag = TC_WANT_TYPE, .from = wanted_fn_type, .to = callee_ind},
          // Can't be more specific, as the current node might be wanted=unknown
          {
            .tag = TC_NODE_MATCHES,
            .node_ind = callee_ind,
          },
        };
        push_actions(&state, STATIC_LEN(actions), actions);
        break;
      }

      case TC_CALL_POST_CALLEE: {
        node_ind_t callee_ind = PT_CALL_CALLEE_IND(node_params.node);
        node_ind_t param_ind = PT_CALL_PARAM_IND(node_params.node);
        node_ind_t callee_type_ind = state.types.node_types[callee_ind];
        type callee_type = VEC_GET(state.types.types, callee_type_ind);
        switch (callee_type.tag) {
          case T_FN: {
            node_ind_t callee_param_type_ind = T_FN_RET_IND(callee_type);
            node_ind_t callee_ret_type_ind = T_FN_RET_IND(callee_type);
            if (callee_ret_type_ind != node_params.wanted_ind) {
              tc_error err = {.type = TYPE_MISMATCH,
                              .expected = node_params.wanted_ind,
                              .got = callee_ret_type_ind,
                              .pos = node_params.node_ind};
              push_tc_err(&state, err);
              break;
            }
            tc_action actions[] = {
              {.tag = TC_WANT_TYPE,
               .to = param_ind,
               .from = callee_param_type_ind},
              {
                .tag = TC_NODE_MATCHES,
                .node_ind = param_ind,
              },
            };
            push_actions(&state, STATIC_LEN(actions), actions);
            break;
          }
          default: {
            tc_error err = {
              .type = CALLED_NON_FUNCTION,
              .got = callee_type_ind,
              .pos = callee_ind,
            };
            push_tc_err(&state, err);
            break;
          }
        }
        break;
      }

      // We have no wanted information, so ambiguity will error.
      // Used to assign types based on syntax.
      case TC_NODE_UNAMBIGUOUS:
        switch (node_params.node.type) {
          // node_unambiguous
          case PT_SIG:
          case PT_LET:
            give_up("Unexpected non-block-level construct");
            break;

          // node_unambiguous
          case PT_FUN_BODY:
            typecheck_block(&state, node_params, false);
            break;

          // node_unambiguous
          case PT_ROOT:
            typecheck_block(&state, node_params, true);
            break;

          // node_unambiguous
          case PT_UNIT:
            state.types.node_types[node_params.node_ind] =
              mk_primitive_type(&state, T_UNIT);
            break;

          // node_unambiguous
          case PT_CONSTRUCTION:
            UNIMPLEMENTED("Typechecking constructors");
            break;

          // node_unambiguous
          case PT_STRING:
            state.types.node_types[node_params.node_ind] =
              state.string_type_ind;
            break;

          // node_unambiguous
          case PT_LIST:
            push_list_subs_unambiguous(&state, node_params, TC_NODE_UNAMBIGUOUS,
                                       TC_NODE_MATCHES);
            break;

          // node_unambiguous
          case PT_AS:
            tc_as(&state, node_params);
            break;

          // node_unambiguous
          case PT_TUP:
            push_tuple_subs(&state, node_params, TC_NODE_UNAMBIGUOUS);
            tc_action action = {.tag = TC_RECONSTRUCT_TUPLE,
                                .node_ind = node_params.node_ind};
            push_action(&state, action);
            break;

          // node_unambiguous
          case PT_IF: {
            node_ind_t cond = PT_IF_COND_IND(state.tree.inds, node_params.node);
            node_ind_t b1 = PT_IF_A_IND(state.tree.inds, node_params.node);
            node_ind_t b2 = PT_IF_B_IND(state.tree.inds, node_params.node);

            const tc_action actions[] = {
              {.tag = TC_WANT_TYPE, .from = state.bool_type_ind, .to = cond},
              {
                .tag = TC_NODE_MATCHES,
                .node_ind = cond,
              },
              {
                .tag = TC_NODE_UNAMBIGUOUS,
                .node_ind = b1,
              },
              {.tag = TC_CLONE_ACTUAL_WANTED, .from = b1, .to = b2},
              {
                .tag = TC_NODE_MATCHES,
                .node_ind = b2,
              },
              {.tag = TC_CLONE_ACTUAL_ACTUAL,
               .from = b2,
               .to = node_params.node_ind}};

            push_actions(&state, STATIC_LEN(actions), actions);
            break;
          }

          // node_unambiguous
          case PT_INT: {
            tc_error err = {
              .type = AMBIGUOUS_TYPE,
              .pos = node_params.node_ind,
            };
            push_tc_err(&state, err);
            break;
          }

          // node_unambiguous
          case PT_UPPER_NAME:
          case PT_LOWER_NAME: {
            binding b = node_params.node.span;
            size_t ind = lookup_str_ref(state.input, state.term_env,
                                        state.term_is_builtin, b);
            if (ind == state.term_env.len) {
              tc_error err = {
                .type = BINDING_NOT_FOUND,
                .pos = node_params.node_ind,
              };
              push_tc_err(&state, err);
              break;
            }
            node_ind_t type_ind = VEC_GET(state.term_type_inds, ind);
            tc_action action = {.tag = TC_ASSIGN_TYPE,
                                .from = type_ind,
                                .to = node_params.node_ind};
            push_action(&state, action);
            break;
          }

          // node_unambiguous
          case PT_FN: {
            node_ind_t param_ind = PT_FN_PARAM_IND(node_params.node);
            node_ind_t body_ind = PT_FN_BODY_IND(node_params.node);

            // TODO This is wrong. Need to add node_params to scope.
            tc_action actions[] = {
              {
                .tag = TC_NODE_UNAMBIGUOUS,
                .node_ind = param_ind,
              },
              {
                .tag = TC_NODE_UNAMBIGUOUS,
                .node_ind = body_ind,
              },
              {.tag = TC_POP_VARS_TO, .amt = state.term_env.len},
              {
                .tag = TC_NODE_UNAMBIGUOUS_FN_STAGE_TWO,
                .node_ind = node_params.node_ind,
              }};
            push_actions(&state, STATIC_LEN(actions), actions);
            break;
          }

          // node_unambiguous
          case PT_FUN: {
            node_ind_t param_ind =
              PT_FUN_PARAM_IND(state.tree.inds, node_params.node);
            node_ind_t body_ind =
              PT_FUN_BODY_IND(state.tree.inds, node_params.node);

            tc_action actions[] = {
              {
                .tag = TC_NODE_UNAMBIGUOUS,
                .node_ind = param_ind,
              },
              {
                .tag = TC_NODE_UNAMBIGUOUS,
                .node_ind = body_ind,
              },
              {.tag = TC_POP_VARS_TO, .amt = state.term_env.len},
              {
                .tag = TC_NODE_UNAMBIGUOUS_FUN_STAGE_TWO,
                .node_ind = node_params.node_ind,
              }};
            push_actions(&state, STATIC_LEN(actions), actions);
            break;
          }

          // node_unambiguous
          case PT_CALL:
            tc_call(&state, node_params);
            break;

          // node_unambiguous
          case PT_LIST_TYPE:
          case PT_FN_TYPE:
            give_up("Can't typecheck term");
            break;
        }
        break;

      case TC_NODE_UNAMBIGUOUS_FN_STAGE_TWO: {
        node_ind_t param_ind = PT_FN_PARAM_IND(node_params.node);
        node_ind_t body_ind = PT_FN_BODY_IND(node_params.node);
        state.types.node_types[node_params.node_ind] =
          mk_type_inline(&state, T_FN, state.types.node_types[param_ind],
                         state.types.node_types[body_ind]);
        break;
      }

      case TC_NODE_UNAMBIGUOUS_FUN_STAGE_TWO: {
        node_ind_t bnd_ind =
          PT_FUN_BINDING_IND(state.tree.inds, node_params.node);
        node_ind_t param_ind =
          PT_FUN_PARAM_IND(state.tree.inds, node_params.node);
        node_ind_t body_ind =
          PT_FUN_BODY_IND(state.tree.inds, node_params.node);

        // TODO consider using wanted value?
        node_ind_t type =
          mk_type_inline(&state, T_FN, state.types.node_types[param_ind],
                         state.types.node_types[body_ind]);
        state.types.node_types[node_params.node_ind] = type;
        state.types.node_types[bnd_ind] = type;
        break;
      }

      case TC_NODE_MATCHES:
        // Here, we're typechecking a node against 'wanted' (an explicit type
        // given to the node) Hence, we shouldn't need a combine two_stage, we
        // just assign the 'wanted' type.
        switch (node_params.node.type) {
          // node_matches
          case PT_SIG:
          case PT_LET:
            give_up("Unexpected non-block-level construct");
            break;

          // node_matches
          case PT_FUN_BODY:
            typecheck_block(&state, node_params, false);
            break;

          // node_matches
          case PT_ROOT:
            typecheck_block(&state, node_params, true);
            break;

          // node_matches
          case PT_UNIT:
            if (node_params.wanted.tag != T_UNIT) {
              tc_error err = {
                .type = LITERAL_MISMATCH,
                .pos = node_params.node_ind,
                .expected = node_params.wanted_ind,
              };
              push_tc_err(&state, err);
            }
            break;

          // node_matches
          case PT_CONSTRUCTION:
            UNIMPLEMENTED("Typechecking constructors");
            break;

          // node_matches
          case PT_STRING:
            if (node_params.wanted_ind != state.string_type_ind) {
              tc_error err = {
                .type = LITERAL_MISMATCH,
                .pos = node_params.node_ind,
                .expected = node_params.wanted_ind,
              };
              push_tc_err(&state, err);
            }
            break;

          // node_matches
          case PT_LIST:
            push_list_subs_match(&state, node_params, TC_NODE_MATCHES);
            break;

          // node_matches
          case PT_AS: {
            tc_as(&state, node_params);
            tc_action action = {
              .tag = TC_NODE_MATCHES_AS_STAGE_TWO,
              .node_ind = node_params.node_ind,
            };
            push_action(&state, action);
            break;
          }

          // node_matches
          case PT_TUP: {
            if (node_params.wanted.tag != T_TUP) {
              tc_error err = {
                .type = TYPE_HEAD_MISMATCH,
                .pos = node_params.node_ind,
                .expected = node_params.wanted_ind,
                .got_type_head = T_TUP,
              };
              push_tc_err(&state, err);
              break;
            }
            tc_action actions[4] = {{
                                      .tag = TC_WANT_TYPE,
                                      .from = T_TUP_SUB_A(node_params.wanted),
                                      .to = PT_TUP_SUB_A(node_params.node),
                                    },
                                    {
                                      .tag = TC_WANT_TYPE,
                                      .from = T_TUP_SUB_B(node_params.wanted),
                                      .to = PT_TUP_SUB_B(node_params.node),
                                    }};
            push_actions(&state, STATIC_LEN(actions), actions);
            push_tuple_subs(&state, node_params, TC_NODE_MATCHES);
            break;
          }

          // node_matches
          case PT_IF: {
            node_ind_t cond = PT_IF_COND_IND(state.tree.inds, node_params.node);
            node_ind_t b1 = PT_IF_A_IND(state.tree.inds, node_params.node);
            node_ind_t b2 = PT_IF_B_IND(state.tree.inds, node_params.node);

            const tc_action actions[] = {
              {.tag = TC_WANT_TYPE, .from = state.bool_type_ind, .to = cond},
              {
                .tag = TC_NODE_MATCHES,
                .node_ind = cond,
              },
              {.tag = TC_CLONE_WANTED_WANTED,
               .from = node_params.node_ind,
               .to = b1},
              {
                .tag = TC_NODE_MATCHES,
                .node_ind = b1,
              },
              // second branch wanted == first branch
              {.tag = TC_CLONE_ACTUAL_WANTED, .from = b1, .to = b2},
              {
                .tag = TC_NODE_MATCHES,
                .node_ind = b2,
              },
            };

            push_actions(&state, STATIC_LEN(actions), actions);
            break;
          }

          // node_matches
          case PT_INT:
            check_int_fits_type(&state, node_params.node_ind,
                                node_params.wanted_ind);
            break;

          // node_matches
          case PT_UPPER_NAME:
          case PT_LOWER_NAME: {
            binding b = node_params.node.span;
            size_t ind = lookup_str_ref(state.input, state.term_env,
                                        state.term_is_builtin, b);
            if (ind == state.term_env.len) {
              tc_error err = {
                .type = BINDING_NOT_FOUND,
                .pos = node_params.node_ind,
              };
              push_tc_err(&state, err);
              break;
            }
            node_ind_t type_ind = VEC_GET(state.term_type_inds, ind);
            if (node_params.wanted_ind != type_ind) {
              tc_error err = {
                .type = TYPE_MISMATCH,
                .pos = node_params.node_ind,
                .expected = node_params.wanted_ind,
                .got = type_ind,
              };
              push_tc_err(&state, err);
            }
            break;
          }

          // node_matches
          case PT_FN: {
            node_ind_t param_ind = PT_FN_PARAM_IND(node_params.node);
            node_ind_t body_ind = PT_FN_BODY_IND(node_params.node);
            if (node_params.wanted.tag == T_FN) {
              tc_action actions[] = {
                {.tag = TC_WANT_TYPE,
                 .from = T_FN_PARAM_IND(node_params.wanted),
                 .to = param_ind},
                {
                  .tag = TC_NODE_MATCHES,
                  .node_ind = param_ind,
                },
                {.tag = TC_WANT_TYPE,
                 .from = T_FN_RET_IND(node_params.wanted),
                 .to = body_ind},
                {
                  .tag = TC_NODE_MATCHES,
                  .node_ind = body_ind,
                },
                {.tag = TC_POP_VARS_TO, .amt = state.term_env.len}};
              push_actions(&state, STATIC_LEN(actions), actions);
              break;
            }
            tc_error err = {
              .type = TYPE_HEAD_MISMATCH,
              .expected = node_params.wanted_ind,
              .got_type_head = T_FN,
              .pos = node_params.node_ind,
            };
            push_tc_err(&state, err);
            break;
          }

          // node_matches
          case PT_FUN: {
            node_ind_t bnd_ind =
              PT_FUN_BINDING_IND(state.tree.inds, node_params.node);
            node_ind_t param_ind =
              PT_FUN_PARAM_IND(state.tree.inds, node_params.node);
            node_ind_t body_ind =
              PT_FUN_BODY_IND(state.tree.inds, node_params.node);

            if (node_params.wanted.tag == T_FN) {
              tc_action actions[] = {
                {.tag = TC_CLONE_ACTUAL_ACTUAL,
                 .from = node_params.node_ind,
                 .to = bnd_ind},
                {.tag = TC_WANT_TYPE,
                 .from = T_FN_PARAM_IND(node_params.wanted),
                 .to = param_ind},
                {
                  .tag = TC_PATTERN_MATCHES,
                  .node_ind = param_ind,
                },
                {.tag = TC_WANT_TYPE,
                 .from = T_FN_RET_IND(node_params.wanted),
                 .to = body_ind},
                {
                  .tag = TC_NODE_MATCHES,
                  .node_ind = body_ind,
                },
                {.tag = TC_POP_VARS_TO, .amt = state.term_env.len},
              };
              push_actions(&state, STATIC_LEN(actions), actions);
              break;
            }
            tc_error err = {.type = TYPE_HEAD_MISMATCH,
                            .expected = node_params.wanted_ind,
                            .got_type_head = T_FN,
                            .pos = node_params.node_ind};
            push_tc_err(&state, err);
            break;
          }

          // node_matches
          case PT_CALL:
            tc_call(&state, node_params);
            break;

          // node_matches
          case PT_LIST_TYPE:
          case PT_FN_TYPE:
            give_up("Type node at term-level");
            break;
        }
        break;

      case TC_NODE_MATCHES_AS_STAGE_TWO: {
        node_ind_t sig_ind = PT_AS_TYPE_IND(node_params.node);
        node_ind_t sig_type_ind = state.types.node_types[sig_ind];
        if (node_params.wanted_ind != sig_type_ind) {
          tc_error err = {
            .type = TYPE_MISMATCH,
            .pos = node_params.node_ind,
            .expected = node_params.wanted_ind,
            .got = sig_type_ind,
          };
          push_tc_err(&state, err);
        }
        break;
      }

      case TC_PATTERN_MATCHES:
        tc_pattern_matches(&state, node_params);
        break;

      case TC_PATTERN_UNAMBIGUOUS:
        tc_pattern_unambiguous(&state, node_params);
        break;

      case TC_RECONSTRUCT_TUPLE: {
        node_ind_t sub_a = PT_TUP_SUB_A(node_params.node);
        node_ind_t sub_b = PT_TUP_SUB_B(node_params.node);
        node_ind_t type_a = state.types.node_types[sub_a];
        node_ind_t type_b = state.types.node_types[sub_b];
        state.types.node_types[node_params.node_ind] =
          mk_type_inline(&state, T_TUP, type_a, type_b);
        break;
      }

      case TC_RECONSTRUCT_LIST: {
        node_ind_t *inds = state.tree.inds;
        node_ind_t sub_amt = PT_LIST_SUB_AMT(node_params.node);
        if (sub_amt == 0)
          break;
        node_ind_t sub_ind = PT_LIST_SUB_IND(inds, node_params.node, 0);
        node_ind_t sub_type_ind = state.types.node_types[sub_ind];
        if (sub_type_ind == state.unknown_ind)
          break;
        state.types.node_types[node_params.node_ind] =
          mk_type_inline(&state, T_LIST, sub_type_ind, 0);
        break;
      }

      // fills in a type from a parse_tree type
      case TC_TYPE: {
        node_ind_t node_ind = node_params.node_ind;
        parse_node node = state.tree.nodes[node_ind];

        switch (node.type) {
          // tc_type
          case PT_LIST_TYPE: {
            {
              tc_action action = {
                .tag = TC_TYPE,
                .node_ind = PT_LIST_TYPE_SUB(node),
              };
              push_action(&state, action);
            }
            tc_action action = {
              .tag = TC_TYPE_LIST_STAGE_TWO,
              .node_ind = node_ind,
            };
            push_action(&state, action);
            break;
          }

          // tc_type
          case PT_TUP: {
            tc_action actions[3] = {
              {.tag = TC_TYPE, .node_ind = PT_TUP_SUB_A(node)},
              {.tag = TC_TYPE, .node_ind = PT_TUP_SUB_B(node)},
              {.tag = TC_RECONSTRUCT_TUPLE, .node_ind = node_ind}};
            push_actions(&state, STATIC_LEN(actions), actions);
            break;
          }

          // tc_type
          case PT_FN_TYPE: {
            node_ind_t param_ind = PT_FN_TYPE_PARAM_IND(node);
            node_ind_t return_ind = PT_FN_TYPE_RETURN_IND(node);
            tc_action actions[] = {
              {
                .tag = TC_TYPE,
                .node_ind = param_ind,
              },
              {
                .tag = TC_TYPE,
                .node_ind = return_ind,
              },
              {
                .tag = TC_TYPE_FN_STAGE_TWO,
                .node_ind = node_ind,
              },
            };
            push_actions(&state, STATIC_LEN(actions), actions);
            break;
          }

          // tc_type
          case PT_UPPER_NAME: {
            binding b = {.start = node.span.start, .end = node.span.end};
            size_t ind = lookup_str_ref(state.input, state.type_env,
                                        state.type_is_builtin, b);
            if (ind == state.type_env.len) {
              tc_error err = {
                .type = TYPE_NOT_FOUND,
                .pos = node_ind,
              };
              push_tc_err(&state, err);
              break;
            }
            node_ind_t type_ind = VEC_GET(state.type_type_inds, ind);
            state.types.node_types[node_ind] = type_ind;
            break;
          }

          // tc_type
          case PT_UNIT: {
            node_ind_t type_ind = mk_primitive_type(&state, T_UNIT);
            state.types.node_types[node_ind] = type_ind;
            break;
          }

          // tc_type
          case PT_CALL: {
            node_ind_t callee_ind = PT_CALL_CALLEE_IND(node);
            node_ind_t param_ind = PT_CALL_PARAM_IND(node);
            tc_action actions[] = {
              {
                .tag = TC_TYPE,
                .node_ind = callee_ind,
              },
              {
                .tag = TC_TYPE,
                .node_ind = param_ind,
              },
              {
                .tag = TC_TYPE_CALL_STAGE_TWO,
                .node_ind = node_ind,
              },
            };
            push_actions(&state, STATIC_LEN(actions), actions);
            break;
          }

          // tc_type
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
          case PT_LIST:
          case PT_LET:
            give_up("Tried creating type from unsupported node");
            break;
        }
        break;
      }

      case TC_TYPE_LIST_STAGE_TWO: {
        state.types.node_types[node_params.node_ind] = mk_type_inline(
          &state, T_LIST,
          state.types.node_types[PT_LIST_TYPE_SUB(node_params.node)], 0);
        break;
      }

      case TC_TYPE_FN_STAGE_TWO: {
        node_ind_t param_ind = PT_FN_TYPE_PARAM_IND(node_params.node);
        node_ind_t return_ind = PT_FN_TYPE_RETURN_IND(node_params.node);
        state.types.node_types[action.node_ind] =
          mk_type_inline(&state, T_FN, state.types.node_types[param_ind],
                         state.types.node_types[return_ind]);
        break;
      }

      case TC_TYPE_CALL_STAGE_TWO: {
        node_ind_t callee_ind = PT_CALL_CALLEE_IND(node_params.node);
        node_ind_t callee_type_ind = state.types.node_types[callee_ind];
        type callee = VEC_GET(state.types.types, callee_type_ind);
        switch (callee.tag) {
          case T_UNKNOWN:
            break;
          case T_FN: {
            node_ind_t callee_return_type_ind = T_FN_RET_IND(callee);
            state.types.node_types[action.node_ind] = callee_return_type_ind;
            break;
          }
          default: {
            node_ind_t from_to[2] = {state.unknown_ind, state.unknown_ind};
            node_ind_t exp =
              mk_type(&state, T_FN, from_to, STATIC_LEN(from_to));
            tc_error err = {.type = CALLED_NON_FUNCTION,
                            .pos = action.node_ind,
                            .expected = exp,
                            .got = callee_ind};
            push_tc_err(&state, err);
            break;
          }
        }
        break;
      }

      case TC_ASSIGN_TYPE:
        state.types.node_types[action.to] = action.from;
        break;

      case TC_WANT_TYPE:
        state.wanted[action.to] = action.from;
        break;

      case TC_RECOVER: {
        tc_action a = VEC_POP(&state.stack);
        tc_action b = VEC_POP(&state.stack);
        size_t err_diff = state.errors.len - action.amt;
        if (err_diff == 0) {
          VEC_PUSH(&state.stack, a);
        } else {
          VEC_POP_N(&state.errors, err_diff);
          VEC_PUSH(&state.stack, b);
        }
        break;
      }
    }

#ifdef DEBUG_TC
    if (state.errors.len > starting_err_amt) {
      fprintf(debug_out, "Caused an error.\n");
    }
    switch (action.tag) {
      case TC_CLONE_ACTUAL_WANTED:
      case TC_CLONE_WANTED_WANTED:
      case TC_WANT_TYPE:
        fputs("New wanted: ", debug_out);
        print_type(debug_out, VEC_DATA_PTR(&state.types.types),
                   state.wanted[action.to]);
        putc('\n', debug_out);
        break;
      case TC_TYPE:
      case TC_TYPE_LIST_STAGE_TWO:
      case TC_NODE_MATCHES:
      case TC_NODE_MATCHES_AS_STAGE_TWO:
      case TC_NODE_UNAMBIGUOUS:
      case TC_NODE_UNAMBIGUOUS_FN_STAGE_TWO:
      case TC_NODE_UNAMBIGUOUS_FUN_STAGE_TWO:
      case TC_PATTERN_MATCHES:
      case TC_PATTERN_UNAMBIGUOUS:
      case TC_CLONE_ACTUAL_ACTUAL:
      case TC_CLONE_WANTED_ACTUAL:
      case TC_ASSIGN_TYPE:
        fputs("Result: ", debug_out);
        print_type(debug_out, VEC_DATA_PTR(&state.types.types),
                   state.types.node_types[action.node_ind]);
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

  tc_res res = {.error_amt = state.errors.len,
                .errors = VEC_FINALIZE(&state.errors),
                .types = {
                  .type_amt = state.types.types.len,
                  .types = VEC_FINALIZE(&state.types.types),
                  .node_types = state.types.node_types,
                  .type_inds = VEC_FINALIZE(&state.types.type_inds),
                }};
  return res;
}

void free_tc_res(tc_res res) {
  free(res.types.types);
  free(res.types.type_inds);
  free(res.types.node_types);
  if (res.error_amt > 0) {
    free(res.errors);
  }
}
