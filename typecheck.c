#include "typecheck.h"

#include <stdbool.h>

#include "consts.h"
#include "source.h"
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

static bool bnds_eq(source_file source, binding a, binding b) {
  size_t la = a.end - a.start;
  if (la != b.end - b.start)
    return false;
  return strncmp(source.data + a.start, source.data + b.start, la) == 0;
}

static size_t lookup_bnd(source_file source, vec_binding bnds, binding bnd) {
  for (size_t i = 0; i < bnds.len; i++) {
    size_t ind = bnds.len - i;
    if (bnds_eq(source, bnds.data[ind], bnd))
      return ind;
  }
  return bnds.len;
}

static size_t lookup_bnd_node(source_file source, vec_binding bnds,
                              parse_node node) {
  binding bnd = {.start = node.start, .end = node.end};
  return lookup_bnd(source, bnds, bnd);
}

tc_res typecheck(source_file source, parse_tree tree) {

  tc_res res = {.successful = true, .types = VEC_NEW, .errors = VEC_NEW};

  vec_tc_action stack = VEC_NEW;
  VEC_PUSH(&stack, tree.root_ind);
  VEC_REPLICATE(&res.types, T_UNKNOWN, tree.nodes.len);
  VEC_PUSH(&res.types, T_UNKNOWN);

  size_t unknown_type_ind = res.types.len - 1;

  vec_binding env = VEC_NEW;

  while (stack.len > 0) {
    tc_action action = VEC_POP(&stack);
    size_t actions_start = stack.len;

    switch (action.tag) {
      case POP_VARS: {
        env.len -= action.amt;
        break;
      }
      case TC_NODE: {
        parse_node node = tree.nodes.data[action.node_ind];
        switch (node.type) {
          case PT_ROOT: {
            for (size_t i = 0; i < node.sub_amt; i++) {
              tc_action a = {.tag = TC_NODE, .node_ind = node.subs_start + i};
              VEC_PUSH(&stack, a);
            }
            break;
          }
          case PT_LOWER_NAME:
          case PT_UPPER_NAME: {
            size_t ind = lookup_bnd_node(source, env, node);
            if (ind == env.len) {
              res.successful = false;
              tc_error err = {
                .err_type = BINDING_NOT_FOUND,
                .pos = ind,
              };
              VEC_PUSH(&res.errors, err);
            }
            type t = res.types.data[env.data[ind].type_ind];
            res.types.data[action.node_ind] = t;
            break;
          }
          case PT_FN: {
            for (size_t i = 0; i < (node.sub_amt - 1) / 2; i++) {
              NODE_IND_T param_ind = tree.inds.data[node.subs_start + i * 2];
              NODE_IND_T type_ind = tree.inds.data[node.subs_start + i * 2 + 1];
              parse_node param = tree.nodes.data[param_ind];
              binding b = {
                .start = param.start, .end = param.end, .type_ind = type_ind};
              VEC_PUSH(&env, b);
            }
            tc_action a = {.tag = POP_VARS, .amt = (node.sub_amt - 1) / 2};
            VEC_PUSH(&stack, a);
            break;
          }
          case PT_CALL: {

            NODE_IND_T callee_ind = tree.inds.data[node.subs_start];
            type fn_type = res.types.data[callee_ind];
            if (fn_type.tag == T_UNKNOWN) {
              VEC_PUSH(&stack, callee_ind);
              VEC_PUSH(&stack, action.node_ind);
              break;
            }

            NODE_IND_T exp_param_amt = fn_type.sub_amt - 1;
            NODE_IND_T got_param_amt = node.sub_amt - 1;
            if (exp_param_amt != got_param_amt) {
              res.successful = false;
              tc_error err = {
                .err_type = WRONG_ARITY,
                .pos = action.node_ind,
                .exp_param_amt = exp_param_amt,
                .got_param_amt = got_param_amt,
              };
              VEC_PUSH(&res.errors, err);
              goto ret;
            }

            for (size_t i = 1; i < node.sub_amt; i++) {
              VEC_PUSH(&stack, );
            }
          }
        }
      }
    }

    reverse_arbitrary(&stack, MAX(actions_start, stack.len) - actions_start,
                      sizeof(tc_action));
  }

ret:
  VEC_FREE(&bnd_env);
  return res;
}
