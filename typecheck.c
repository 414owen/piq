#include <stdbool.h>
#include <stdlib.h>

#include "bitset.h"
#include "consts.h"
#include "source.h"
#include "typecheck.h"
#include "vec.h"

typedef struct {

  enum {
    POP_VARS,
    TC_NODE,
  } tag;

  union {
    NODE_IND_T amt;
    NODE_IND_T node_ind;
  };

} tc_action;

VEC_DECL(tc_action);

typedef struct {
  BUF_IND_T start;
  BUF_IND_T end;
  NODE_IND_T type_ind;
} binding;

VEC_DECL(binding);

typedef union {
  char *builtin;
  struct {
    BUF_IND_T start;
    BUF_IND_T end;
  };
} str_ref;

VEC_DECL(str_ref);

typedef struct {
  tc_res res;
  parse_tree tree;
  vec_tc_action stack;
  // type names in scope
  vec_str_ref known_type_names;
  // ref to source, or builtin
  bitset type_is_builtin;
  // index into res.types
  vec_node_ind known_type_inds;
  // res.node_types is wanted or known
  bitset is_wanted;
  vec_binding env;
  source_file source;
} typecheck_state;

static bool bnds_eq(typecheck_state *state, binding a, binding b) {
  size_t la = a.end - a.start;
  if (la != b.end - b.start)
    return false;
  return strncmp(state->source.data + a.start, state->source.data + b.start,
                 la) == 0;
}

static size_t lookup_bnd(typecheck_state *state, vec_binding bnds,
                         binding bnd) {
  for (size_t i = 0; i < bnds.len; i++) {
    size_t ind = bnds.len - i;
    if (bnds_eq(state, bnds.data[ind], bnd))
      return ind;
  }
  return bnds.len;
}

static size_t lookup_bnd_node(typecheck_state *state, parse_node node) {
  binding bnd = {.start = node.start, .end = node.end};
  return lookup_bnd(state, state->env, bnd);
}

static void give_up(char *err) {
  fprintf(stderr, "%s.\nThis is a compiler bug! Giving up.", err);
  exit(1);
}

static void setup_type_env(typecheck_state *state) {

  // Nodes have initial type
  VEC_REPLICATE(&state->res.types, T_UNKNOWN, state->tree.nodes.len);
  bs_resize(&state->is_wanted, state->tree.nodes.len);
  state->is_wanted.len = state->tree.nodes.len;
  memset(&state->is_wanted.data, 0, state->is_wanted.cap);

  // Prelude
  {
    static const char *builtin_names[] = {
      "U8", "U16", "U32", "U64", "I8", "I16", "I32", "I64",
    };

    static const type_tag builtin_types[] = {T_U8, T_U16, T_U32, T_U64,
                                             T_I8, T_I16, T_I32, T_I64};

    VEC_APPEND(&state->known_type_names, STATIC_LEN(builtin_names),
               builtin_names);
    size_t start_type_ind = state->res.types.len;
    for (size_t i = 0; i < STATIC_LEN(builtin_types); i++) {
      VEC_PUSH(&state->known_type_inds, start_type_ind + i);
    }
    VEC_APPEND(&state->res.types, STATIC_LEN(builtin_types), builtin_types);
  }

  // Declared here
  {
    for (size_t i = 0; i < state->tree.nodes.len; i++) {
      parse_node node = state->tree.nodes.data[i];
      switch (node.type) {
        // TODO fill in user declared types here
        default:
          break;
      }
    }
  }
}

static void fill_in_type(typecheck_state *state, NODE_IND_T node_ind) {
  parse_node type_node = state->tree.nodes.data[node_ind];
  size_t types_len = state->known_type_names.len;
  switch (type_node.type) {
    case PT_UPPER_NAME:
      for (size_t i = 0; i < types_len; i++) {
        bool is_builtin = bs_get(state->type_is_builtin, types_len - i);
        size_t len = type_node.end - type_node.start;
        str_ref ref = state->known_type_names.data[types_len - i];
        bool match =
          strncmp(state->source.data + type_node.start,
                  is_builtin ? ref.builtin : state->source.data + ref.start,
                  len) == 0;
        if (match) {
          state->res.node_types.data[node_ind] = state->known_type_inds.data[i];
          return;
        }
      }
    default:
      give_up("Can't use non-name as a type yet.");
  }

  tc_error err = {
    .err_type = BINDING_NOT_FOUND,
    .pos = node_ind,
  };
  VEC_PUSH(&state->res.errors, err);
}

static void fill_in_types(typecheck_state *state) {
  for (size_t i = 0; i < state->tree.nodes.len; i++) {
    parse_node node = state->tree.nodes.data[i];
    switch (node.type) {
      case PT_FN:
        for (size_t j = 0; j < (node.sub_amt - 2) / 2; j++) {
          fill_in_type(state,
                       state->tree.inds.data[node.subs_start + j * 2 + 1]);
        }
        fill_in_type(state, state->tree.inds.data[node.subs_start - 2]);
        break;
      case PT_TYPED:
        fill_in_type(state, state->tree.inds.data[node.subs_start + 1]);
        break;
      default:
        break;
    }
  }
}

tc_res typecheck(source_file source, parse_tree tree) {

  typecheck_state state = {
    .res = {.successful = true, .types = VEC_NEW, .errors = VEC_NEW},
    .tree = tree,
    .known_type_names = VEC_NEW,
    .type_is_builtin = bs_new(),
    .known_type_inds = VEC_NEW,
    .is_wanted = bs_new(),
    .stack = VEC_NEW,
    .env = VEC_NEW,
    .source = source,
  };

  VEC_PUSH(&state.stack, tree.root_ind);
  VEC_PUSH(&state.res.types, T_UNKNOWN);
  size_t unknown_type_ind = state.res.types.len - 1;

  setup_type_env(&state);

  while (state.stack.len > 0) {
    tc_action action = VEC_POP(&state.stack);
    size_t actions_start = state.stack.len;

    switch (action.tag) {
      case POP_VARS: {
        state.env.len -= action.amt;
        break;
      }
      case TC_NODE: {
        parse_node node = tree.nodes.data[action.node_ind];
        switch (node.type) {
          case PT_INT: {
          }
          case PT_TOP_LEVEL: {
            give_up("Top level node as child of non-root");
            break;
          }
          case PT_ROOT: {
            for (size_t i = 0; i < node.sub_amt; i++) {
              parse_node toplevel = tree.nodes.data[node.subs_start + i];
              assert(toplevel.type == PT_TOP_LEVEL);
              NODE_IND_T name_ind = tree.inds.data[toplevel.subs_start];
              NODE_IND_T val_ind = tree.inds.data[toplevel.subs_start];
              parse_node val = tree.nodes.data[val_ind];
              switch (val.type) {
                case PT_FN: {

                  break;
                }
                default: {
                  give_up("Unhandled case. This is a compiler bug!");
                }
              }
            }
            break;
          }
          case PT_LOWER_NAME:
          case PT_UPPER_NAME: {
            size_t ind = lookup_bnd_node(&state, node);
            if (ind == state.env.len) {
              state.res.successful = false;
              tc_error err = {
                .err_type = BINDING_NOT_FOUND,
                .pos = ind,
              };
              VEC_PUSH(&state.res.errors, err);
            }
            type t = state.res.types.data[state.env.data[ind].type_ind];
            state.res.types.data[action.node_ind] = t;
            break;
          }
          case PT_FN: {
            for (size_t i = 0; i < (node.sub_amt - 1) / 2; i++) {
              NODE_IND_T param_ind = tree.inds.data[node.subs_start + i * 2];
              NODE_IND_T type_ind = tree.inds.data[node.subs_start + i * 2 + 1];
              parse_node param = tree.nodes.data[param_ind];
              binding b = {
                .start = param.start, .end = param.end, .type_ind = type_ind};
              VEC_PUSH(&state.env, b);
            }
            tc_action a = {.tag = POP_VARS, .amt = (node.sub_amt - 1) / 2};
            VEC_PUSH(&state.stack, a);
            break;
          }
          case PT_CALL: {

            NODE_IND_T callee_ind = tree.inds.data[node.subs_start];
            type fn_type = state.res.types.data[callee_ind];
            if (fn_type.tag == T_UNKNOWN) {
              VEC_PUSH(&state.stack, callee_ind);
              VEC_PUSH(&state.stack, action.node_ind);
              break;
            }

            NODE_IND_T exp_param_amt = fn_type.sub_amt - 1;
            NODE_IND_T got_param_amt = node.sub_amt - 1;
            if (exp_param_amt != got_param_amt) {
              state.res.successful = false;
              tc_error err = {
                .err_type = WRONG_ARITY,
                .pos = action.node_ind,
                .exp_param_amt = exp_param_amt,
                .got_param_amt = got_param_amt,
              };
              VEC_PUSH(&state.res.errors, err);
              goto ret;
            }

            for (size_t i = 1; i < node.sub_amt; i++) {
              // VEC_PUSH(&state.stack, );
            }
          }
        }
      }
    }

    reverse_arbitrary(&state.stack,
                      MAX(actions_start, state.stack.len) - actions_start,
                      sizeof(tc_action));
  }

ret:
  VEC_FREE(&state.env);
  VEC_FREE(&state.stack);
  return state.res;
}
