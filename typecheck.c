#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "binding.h"
#include "bitset.h"
#include "consts.h"
#include "diagnostic.h"
#include "scope.h"
#include "source.h"
#include "typecheck.h"
#include "util.h"
#include "vec.h"

// TODO: replace TC_CLONE_LAST with TC_COMBINE
typedef struct {
  enum {
    TC_POP_VARS,
    TC_NODE,
    // type -> type
    TC_CLONE,
    // type -> wanted
    TC_WANT,
    // wanted -> wanted
    TC_CLONE_WANTED,
    TC_COMBINE,
  } tag;

  union {
    NODE_IND_T amt;
    NODE_IND_T node_ind;
    struct {
      NODE_IND_T from;
      NODE_IND_T to;
    };
  };
} tc_action;

VEC_DECL(tc_action);

typedef struct {
  tc_res res;
  parse_tree tree;
  NODE_IND_T current_node_ind;

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
  // res.node_types is wanted or known
  vec_node_ind wanted;

  NODE_IND_T unknown_ind;
  NODE_IND_T string_type_ind;

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
    if (memcmp(&VEC_DATA_PTR(&state->res.type_inds)[t.sub_start], subs,
               sub_amt) != 0)
      continue;
    return i;
  }
  return state->res.types.len;
}

static NODE_IND_T mk_primitive_type(typecheck_state *state, type_tag tag) {
  NODE_IND_T ind = find_primitive_type(state, tag);
  if (ind < state->res.types.len)
    return ind;
  type t = {
    .tag = tag,
    .sub_amt = 0,
    .sub_start = 0,
  };
  VEC_PUSH(&state->res.types, t);
  return state->res.types.len - 1;
}

static NODE_IND_T mk_type(typecheck_state *state, type_tag tag,
                          NODE_IND_T *subs, NODE_IND_T sub_amt) {
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
    ind = state->res.type_inds.len;
    VEC_APPEND(&state->res.type_inds, sub_amt, subs);
  }
  type t = {
    .tag = tag,
    .sub_amt = sub_amt,
    .sub_start = ind,
  };
  VEC_PUSH(&state->res.types, t);
  return state->res.types.len - 1;
}

// Add builtin types and custom types to type env
static void setup_type_env(typecheck_state *state) {

  {
    type t = {.tag = T_UNKNOWN, .sub_amt = 0, .sub_start = 0};
    VEC_PUSH(&state->res.types, t);
    state->unknown_ind = state->res.types.len - 1;
  }

  // Nodes have initial type UNKNOWN and not is_wanted
  VEC_REPLICATE(&state->res.node_types, state->res.tree.node_amt,
                state->unknown_ind);
  VEC_REPLICATE(&state->wanted, state->res.tree.node_amt, state->unknown_ind);

  // Prelude
  {
    static const char *builtin_names[] = {"U8",  "U16", "U32", "U64",   "I8",
                                          "I16", "I32", "I64", "String"};

    static const type_tag primitive_types[] = {T_U8, T_U16, T_U32, T_U64,
                                               T_I8, T_I16, T_I32, T_I64};

    VEC_APPEND(&state->type_env, STATIC_LEN(builtin_names), builtin_names);
    NODE_IND_T start_type_ind = state->res.types.len;
    for (NODE_IND_T i = 0; i < STATIC_LEN(primitive_types); i++) {
      VEC_PUSH(&state->type_type_inds, start_type_ind + i);
      type t = {.tag = primitive_types[i], .sub_start = 0, .sub_amt = 0};
      VEC_PUSH(&state->res.types, t);
      bs_push(&state->type_is_builtin, true);
    }

    {
      NODE_IND_T u8_ind = mk_type(state, T_U8, NULL, 0);
      NODE_IND_T list_ind = mk_type(state, T_LIST, &u8_ind, 1);
      state->string_type_ind = list_ind;
      VEC_PUSH(&state->type_type_inds, state->res.types.len - 1);
      bs_push(&state->type_is_builtin, true);
    }
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

typedef struct {
  NODE_IND_T a;
  NODE_IND_T b;
} node_ind_tup;

VEC_DECL(node_ind_tup);

// Not needed since we search previous types and type ind ranges
/*
static bool types_equal(typecheck_state *state, NODE_IND_T a, NODE_IND_T b) {
  if (a == b)
    return true;

  vec_node_ind_tup stack = VEC_NEW;
  node_ind_tup initial = {a, b};
  VEC_PUSH(&stack, initial);

  while (stack.len > 0) {
    node_ind_tup tup = VEC_POP(&stack);
    if (tup.a == tup.b)
      continue;
    type ta = VEC_GET(state->res.types, tup.a);
    type tb = VEC_GET(state->res.types, tup.b);
    if (ta.tag != tb.tag || ta.sub_amt != tb.sub_amt) {
      VEC_FREE(&stack);
      return false;
    }
    for (size_t i = 0; i < ta.sub_amt; i++) {
      node_ind_tup new_tup = {ta.sub_start + i, tb.sub_amt + i};
      VEC_PUSH(&stack, new_tup);
    }
  }
  VEC_FREE(&stack);
  return true;
}
*/

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

static binding node_to_binding(parse_node node) {
  binding b = {.start = node.start, .end = node.end};
  return b;
}

static binding node_to_bnd(parse_node a) {
  binding res = {.start = a.start, .end = a.end};
  return res;
}

static void tc_node(typecheck_state *state) {

  NODE_IND_T node_ind = state->current_node_ind;
  parse_node node = state->res.tree.nodes[node_ind];
  NODE_IND_T wanted_ind = VEC_GET(state->wanted, node_ind);
  bool is_wanted = wanted_ind != state->unknown_ind;
  type wanted = VEC_GET(state->res.types, wanted_ind);

  switch (node.type) {
    case PT_ROOT: {
      debug_assert(node.type == PT_ROOT);

      // TODO for mutually recursive functions, we need the function names to be
      // in scope, so we need to reserve type nodes and such
      parse_node last;
      bool can_continue = true;
      for (NODE_IND_T i = 0; i < node.sub_amt; i++) {
        NODE_IND_T toplevel_ind = state->res.tree.inds[node.subs_start + i];
        parse_node toplevel = state->res.tree.nodes[toplevel_ind];
        switch (toplevel.type) {
          case PT_SIG: {
            // TODO
            // NODE_IND_T name_ind = PT_SIG_BINDING_IND(state->res.tree.inds,
            // toplevel); parse_node name = state->res.tree.nodes[name_ind];
          }
          case PT_FUN: {
            NODE_IND_T name_ind =
              PT_FUN_BINDING_IND(state->res.tree.inds, toplevel);
            parse_node name = state->res.tree.nodes[name_ind];
            bool missing_sig = i == 0;

            if (i > 0) {
              NODE_IND_T prev_toplevel_ind =
                state->res.tree.inds[node.subs_start + i - 1];
              parse_node prev_toplevel = state->res.tree.nodes[toplevel_ind];
              switch (prev_toplevel.type) {
                case PT_SIG:
                case PT_FUN: {
                  NODE_IND_T prev_name_ind =
                    PT_FUN_BINDING_IND(state->res.tree.inds, toplevel);
                  parse_node prev_name = state->res.tree.nodes[prev_name_ind];
                  missing_sig |=
                    compare_bnds(state->source.data, node_to_bnd(prev_name),
                                 node_to_bnd(name)) != 0;
                }
                default: {
                  tc_error err = {
                    .type = MISSING_SIG,
                    .pos = node_ind,
                  };
                  push_tc_err(state, err);
                }
              }
            }

            if (missing_sig) {
              tc_error err = {
                .type = MISSING_SIG,
                .pos = node_ind,
              };
              push_tc_err(state, err);
            }

            tc_action action = {.tag = TC_NODE, .node_ind = toplevel_ind};
            VEC_PUSH(&state->stack, action);
            break;
          }
          default: {
            give_up("Invalid top level");
            break;
          }
        }
        last = toplevel;
      }

      if (can_continue) {
        for (NODE_IND_T i = 0; i < node.sub_amt; i++) {
          NODE_IND_T toplevel_ind = state->res.tree.inds[node.subs_start + i];
          parse_node toplevel = state->res.tree.nodes[toplevel_ind];
          switch (toplevel.type) {
            // fun after fun
            case PT_FUN: {
              NODE_IND_T name_ind =
                PT_FUN_BINDING_IND(state->res.tree.inds, toplevel);
              parse_node name = state->res.tree.nodes[name_ind];
              // TODO
              tc_action action = {.tag = TC_NODE, .node_ind = toplevel_ind};
              VEC_PUSH(&state->stack, action);
            }
            // impossible, already checked above
            default:
              break;
          }
          last = toplevel;
        }
      }
    }
    case PT_UNIT: {
      if (is_wanted) {
        if (wanted.tag != T_UNIT) {
          tc_error err = {
            .type = LITERAL_MISMATCH,
            .pos = node_ind,
            .expected = wanted_ind,
          };
          push_tc_err(state, err);
        }
      }
      break;
    }
    case PT_CONSTRUCTION: {
      UNIMPLEMENTED("Typechecking constructors");
      break;
    }
    case PT_FUN_BODY: {
      UNIMPLEMENTED("Typechecking constructors");
      break;
    }
    case PT_STRING: {
      if (is_wanted) {
        if (wanted_ind != state->string_type_ind) {
          tc_error err = {
            .type = LITERAL_MISMATCH,
            .pos = node_ind,
            .expected = wanted_ind,
          };
          push_tc_err(state, err);
        }
      }
      VEC_GET(state->res.node_types, node_ind) = state->string_type_ind;
      break;
    }
    case PT_LIST: {
      if (is_wanted) {
        if (wanted.tag != T_LIST) {
          tc_error err = {
            .type = LITERAL_MISMATCH,
            .pos = node_ind,
            .expected = wanted_ind,
          };
          push_tc_err(state, err);
          break;
        }
      }
#define action_amt 2
      NODE_IND_T sub_ind = state->res.tree.inds[node.subs_start];
      tc_action actions[action_amt] = {
        {.tag = TC_NODE, .node_ind = sub_ind},
        {.tag = TC_COMBINE, .node_ind = node_ind},
      };
      VEC_APPEND(&state->stack, action_amt, actions);
#undef action_amt
      break;
    }

    case PT_AS: {
      NODE_IND_T val_node_ind = state->res.tree.inds[node.subs_start];
      NODE_IND_T type_node_ind = state->res.tree.inds[node.subs_start + 1];

#define action_amt 3
      tc_action actions[action_amt] = {
        {.tag = TC_NODE, .node_ind = type_node_ind},
        {.tag = TC_CLONE, .from = type_node_ind, .to = val_node_ind},
        {.tag = TC_CLONE, .from = type_node_ind, .to = node_ind},
      };
      VEC_APPEND(&state->stack, action_amt, actions);
#undef action_amt
      break;
    }

    case PT_TUP: {
      for (NODE_IND_T i = 0; i < node.sub_amt; i++) {
        tc_action action = {
          .tag = TC_NODE,
          .node_ind = state->res.tree.inds[node.subs_start + i],
        };
        VEC_PUSH(&state->stack, action);
      }
      tc_action action = {.tag = TC_COMBINE, .node_ind = node_ind};
      VEC_PUSH(&state->stack, action);
      if (is_wanted) {
        if (wanted.tag != T_TUP || wanted.sub_amt != node.sub_amt) {
          tc_error err = {
            .type = LITERAL_MISMATCH,
            .pos = node_ind,
            .expected = wanted_ind,
          };
          push_tc_err(state, err);
          state->stack.len -= node.sub_amt + 1;
        }
      }
      break;
    }

    case PT_IF: {
      NODE_IND_T cond = state->res.tree.inds[node.subs_start];
      NODE_IND_T b1 = state->res.tree.inds[node.subs_start + 1];
      NODE_IND_T b2 = state->res.tree.inds[node.subs_start + 2];

      // TODO benchmark: could skip if not is_wanted
      VEC_GET(state->res.node_types, cond) = prim_ind(state, T_BOOL);

#define action_amt 6
      const tc_action actions[action_amt] = {
        {.tag = TC_CLONE_WANTED, .from = node_ind, .to = b1},
        {.tag = TC_NODE, .node_ind = cond},
        {.tag = TC_NODE, .node_ind = b1},
        // second branch wanted == first branch
        {.tag = TC_WANT, .from = b1, .to = b2},
        {.tag = TC_NODE, .node_ind = b2},
        {.tag = TC_COMBINE, .node_ind = node_ind},
      };
      VEC_APPEND(&state->stack, action_amt, actions);
#undef action_amt
      break;
    }

    case PT_INT: {
      if (is_wanted) {
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
        if (fits) {
          VEC_GET(state->res.node_types, node_ind) = wanted_ind;
        }
      } else {
        tc_error err = {
          .type = AMBIGUOUS_TYPE,
          .pos = node_ind,
        };
        push_tc_err(state, err);
      }
      break;
    }

    case PT_TOP_LEVEL: {
      give_up("Top level node as child of non-root");
      break;
    }

    case PT_LOWER_NAME: {
      binding b = node_to_binding(node);
      size_t ind = lookup_bnd(state->source.data, state->term_env,
                              state->term_is_builtin, b);
      if (ind == state->term_env.len) {
        tc_error err = {
          .type = BINDING_NOT_FOUND,
          .pos = node_ind,
        };
        push_tc_err(state, err);
        break;
      }
      NODE_IND_T term_ind = VEC_GET(state->term_type_inds, ind);
      VEC_GET(state->res.node_types, node_ind) = term_ind;
      break;
    }

    case PT_UPPER_NAME: {
      binding b = {.start = node.start, .end = node.end};
      size_t ind = lookup_bnd(state->source.data, state->type_env,
                              state->type_is_builtin, b);
      NODE_IND_T type_ind = VEC_GET(state->type_type_inds, ind);
      if (ind == state->type_env.len) {
        tc_error err = {
          .type = TYPE_NOT_FOUND,
          .pos = node_ind,
        };
        push_tc_err(state, err);
      }
      VEC_GET(state->res.node_types, node_ind) = type_ind;
      break;
    }

    case PT_FUN: {
      NODE_IND_T param_amt = (node.sub_amt - 2) / 2;
      for (uint16_t i = 0; i < param_amt; i++) {
        NODE_IND_T type_ind = state->res.tree.inds[node.subs_start + i * 2 + 1];
        NODE_IND_T param_ind = state->res.tree.inds[node.subs_start + i * 2];
#define action_amt 2
        const tc_action actions[action_amt] = {
          {.tag = TC_NODE, .node_ind = type_ind},
          {.tag = TC_CLONE, .from = type_ind, .to = param_ind},
        };
        VEC_APPEND(&state->stack, action_amt, actions);
#undef action_amt
      }
      NODE_IND_T ret_type_ind =
        state->res.tree.inds[node.subs_start + node.sub_amt - 2];
      NODE_IND_T body_ind =
        state->res.tree.inds[node.subs_start + node.sub_amt - 1];
#define action_amt 4
      const tc_action actions[action_amt] = {
        {.tag = TC_NODE, .node_ind = ret_type_ind},
        {.tag = TC_WANT, .from = ret_type_ind, .to = body_ind},
        {.tag = TC_NODE, .node_ind = body_ind},
        {.tag = TC_COMBINE, .node_ind = node_ind},
      };
      VEC_APPEND(&state->stack, action_amt, actions);
#undef action_amt
      break;
    }

    case PT_CALL: {
      NODE_IND_T callee_ind = state->res.tree.inds[node.subs_start];
      type fn_type = VEC_GET(state->res.types, callee_ind);
      if (fn_type.tag == T_UNKNOWN) {
        VEC_PUSH(&state->stack, callee_ind);
        VEC_PUSH(&state->stack, node_ind);
        break;
      }

      NODE_IND_T exp_param_amt = fn_type.sub_amt - 1;
      NODE_IND_T got_param_amt = node.sub_amt - 1;
      if (exp_param_amt != got_param_amt) {
        tc_error err = {
          .type = WRONG_ARITY,
          .pos = node_ind,
          .exp_param_amt = exp_param_amt,
          .got_param_amt = got_param_amt,
        };
        push_tc_err(state, err);
      }

      for (size_t i = 1; i < node.sub_amt; i++) {
        // VEC_PUSH(&state.stack, );
      }
    }
  }
}

static typecheck_state tc_new_state(source_file source, parse_tree tree) {
  typecheck_state state = {
    .res = {.types = VEC_NEW,
            .errors = VEC_NEW,
            .source = source,
            .tree = tree},
    .wanted = VEC_NEW,

    .type_env = VEC_NEW,
    .type_is_builtin = bs_new(),
    .type_type_inds = VEC_NEW,

    .term_env = VEC_NEW,
    .term_is_builtin = bs_new(),
    .term_type_inds = VEC_NEW,

    .stack = VEC_NEW,

    .source = source,
  };
  tc_action action = {.tag = TC_NODE, .node_ind = tree.root_ind};
  VEC_PUSH(&state.stack, action);
  return state;
}

static void tc_combine(typecheck_state *state) {
  NODE_IND_T node_ind = state->current_node_ind;
  parse_node node = state->res.tree.nodes[node_ind];
  NODE_IND_T wanted_ind = VEC_GET(state->wanted, node_ind);
  bool is_wanted = wanted_ind != state->unknown_ind;

  NODE_IND_T type_ind;

  switch (node.type) {

    case PT_FUN: {
      const NODE_IND_T param_amt = (node.sub_amt - 2) / 2;
      const size_t param_bytes = sizeof(NODE_IND_T) * param_amt + 1;
      NODE_IND_T *sub_type_inds = stalloc(param_bytes);
      for (NODE_IND_T i = 0; i < param_amt; i++) {
        NODE_IND_T sub_ind = state->res.tree.inds[node.subs_start + i * 2 + 1];
        sub_type_inds[i] = VEC_GET(state->res.node_types, sub_ind);
      }
      NODE_IND_T sub_ind =
        state->res.tree.inds[node.subs_start + node.sub_amt - 2];
      sub_type_inds[param_amt] = VEC_GET(state->res.node_types, sub_ind);

      type_ind = mk_type(state, T_FN, sub_type_inds, param_amt + 1);
      VEC_GET(state->res.node_types, node_ind) = type_ind;
      stfree(sub_type_inds, param_bytes);
      break;
    }

    case PT_TUP: {
      size_t sub_bytes = sizeof(NODE_IND_T) * node.sub_amt + 1;
      NODE_IND_T *sub_type_inds = stalloc(sub_bytes);
      for (NODE_IND_T i = 0; i < node.sub_amt; i++) {
        NODE_IND_T sub_ind = state->res.tree.inds[node.subs_start + i * 2 + 1];
        sub_type_inds[i] = VEC_GET(state->res.node_types, sub_ind);
      }
      type_ind = mk_type(state, T_FN, sub_type_inds, node.sub_amt);
      VEC_GET(state->res.node_types, node_ind) = type_ind;
      stfree(sub_type_inds, sub_bytes);
      break;
    }

    case PT_LIST: {
      NODE_IND_T sub_type_ind =
        state->res.node_types.data[state->res.tree.inds[node.subs_start]];
      type_ind = mk_type(state, T_LIST, &sub_type_ind, 1);
      VEC_GET(state->res.node_types, node_ind) = type_ind;
      break;
    }

    default:
      give_up("Can't combine type");
      return;
  }

  if (is_wanted && type_ind != wanted_ind) {
    tc_error err = {
      .type = TYPE_MISMATCH,
      .pos = node_ind,
      .expected = wanted_ind,
      .got = type_ind,
    };
    push_tc_err(state, err);
  }
}

#ifdef DEBUG_TC
static const char *const action_str[] = {
  [TC_NODE] = "TC Node",
  [TC_COMBINE] = "Combine",
  [TC_CLONE] = "Clone",
  [TC_WANT] = "Want",
  [TC_CLONE_WANTED] = "Clone wanted",
  [TC_POP_VARS] = "Pop vars",
};
#endif

tc_res typecheck(source_file source, parse_tree tree) {
  typecheck_state state = tc_new_state(source, tree);
  setup_type_env(&state);
  // resolve_types(&state);
#ifdef DEBUG_TC
  FILE *debug_out = fopen("debug-typechecker", "a");
#endif

  while (state.stack.len > 0) {
    tc_action action = VEC_POP(&state.stack);
    size_t actions_start = state.stack.len;

#ifdef DEBUG_TC
    {
      fprintf(debug_out, "\nAction: '%s'\n", action_str[action.tag]);
      switch (action.tag) {
        case TC_POP_VARS:
          break;
        case TC_CLONE:
        case TC_WANT:
        case TC_CLONE_WANTED: {
          parse_node node = VEC_GET(tree.nodes, action.from);
          format_error_ctx(debug_out, source.data, node.start, node.end);
          node = VEC_GET(tree.nodes, action.to);
          format_error_ctx(debug_out, source.data, node.start, node.end);
          break;
        }
        default: {
          fprintf(debug_out, "\nNode ind: '%d'\n", action.node_ind);
          parse_node node = VEC_GET(tree.nodes, action.node_ind);
          fprintf(debug_out, "Node: '%s'\n", parse_node_strings[node.type]);
          format_error_ctx(debug_out, source.data, node.start, node.end);
          break;
        }
      }
    }
    size_t starting_err_amt = state.res.errors.len;
#endif

    switch (action.tag) {
      case TC_CLONE:
        VEC_GET(state.res.node_types, action.to) =
          VEC_GET(state.res.node_types, action.from);
        break;
      case TC_WANT:
        VEC_GET(state.wanted, action.to) =
          VEC_GET(state.res.node_types, action.from);
        break;
      case TC_CLONE_WANTED:
        VEC_GET(state.wanted, action.to) = VEC_GET(state.wanted, action.from);
        break;
      case TC_COMBINE:
        state.current_node_ind = action.node_ind;
        tc_combine(&state);
        break;
      case TC_POP_VARS:
        state.term_env.len -= action.amt;
        break;
      case TC_NODE:
        state.current_node_ind = action.node_ind;
        tc_node(&state);
        break;
    }

#ifdef DEBUG_TC
    if (state.res.errors.len > starting_err_amt) {
      fprintf(debug_out, "Caused an error.\n", action_str[action.tag]);
    }
#endif

    reverse_arbitrary(&VEC_GET(state.stack, actions_start),
                      MAX(actions_start, state.stack.len) - actions_start,
                      sizeof(tc_action));
  }

#ifdef DEBUG_TC
  for (size_t i = 0; i < tree.nodes.len; i++) {
    debug_assert(VEC_GET(state.res.node_types, i) != T_UNKNOWN);
  }
#endif

  VEC_FREE(&state.stack);

  VEC_FREE(&state.type_env);
  bs_free(&state.type_is_builtin);
  VEC_FREE(&state.type_type_inds);

  VEC_FREE(&state.term_env);
  bs_free(&state.term_is_builtin);
  VEC_FREE(&state.term_type_inds);
  VEC_FREE(&state.wanted);

#ifdef DEBUG_TC
  fclose(debug_out);
#endif

  return state.res;
}
