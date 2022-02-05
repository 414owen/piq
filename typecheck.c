#include <stdbool.h>
#include <stdlib.h>

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
    struct {
      NODE_IND_T node_ind;
      NODE_IND_T type_goal;
    };
    NODE_IND_T amt;
  };

} tc_action;

VEC_DECL(tc_action);

typedef struct {
  BUF_IND_T start;
  BUF_IND_T end;
  NODE_IND_T type_ind;
} binding;

VEC_DECL(binding);

typedef struct {
  tc_res res;
  vec_tc_action stack;
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

tc_res typecheck(source_file source, parse_tree tree) {

  typecheck_state state = {
    .res = {.successful = true, .types = VEC_NEW, .errors = VEC_NEW},
    .stack = VEC_NEW,
    .env = VEC_NEW,
    .source = source,
  };

  vec_tc_action stack = VEC_NEW;
  VEC_PUSH(&stack, tree.root_ind);
  VEC_REPLICATE(&state.res.types, T_UNKNOWN, tree.nodes.len);
  VEC_PUSH(&state.res.types, T_UNKNOWN);
  size_t unknown_type_ind = state.res.types.len - 1;

  while (stack.len > 0) {
    tc_action action = VEC_POP(&stack);
    size_t actions_start = stack.len;

    switch (action.tag) {
      case POP_VARS: {
        state.env.len -= action.amt;
        break;
      }
      case TC_NODE: {
        parse_node node = tree.nodes.data[action.node_ind];
        switch (node.type) {
          case PT_INT: {
            switch
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
            VEC_PUSH(&stack, a);
            break;
          }
          case PT_CALL: {

            NODE_IND_T callee_ind = tree.inds.data[node.subs_start];
            type fn_type = state.res.types.data[callee_ind];
            if (fn_type.tag == T_UNKNOWN) {
              VEC_PUSH(&stack, callee_ind);
              VEC_PUSH(&stack, action.node_ind);
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

    reverse_arbitrary(&stack, MAX(actions_start, stack.len) - actions_start,
                      sizeof(tc_action));
  }

ret:
  VEC_FREE(&env);
  VEC_FREE(&stack);
  return state.res;
}
