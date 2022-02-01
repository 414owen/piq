#include "typecheck.h"

#include <stdbool.h>

#include "consts.h"
#include "source.h"
#include "vec.h"

typedef enum {
  POP_VAR,
  TC_NODE,
} tc_action;

typedef struct {
  BUF_IND_T start;
  BUF_IND_T end;
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

static void mk_bnd(vec_binding *bnds, vec_type *types, NODE_IND_T bnd,
                   NODE_IND_T type) {
  VEC_PUSH(bnds, bnd);
  VEC_PUSH(types, type);
}

tc_res typecheck(source_file source, parse_tree tree) {
  tc_res res = {.successful = true, .types = VEC_NEW, .errors = VEC_NEW};
  vec_node_ind stack = VEC_NEW;
  VEC_PUSH(&stack, tree.root_ind);
  VEC_REPLICATE(&res.types, T_UNKNOWN, tree.nodes.len);

  vec_type type_env = VEC_NEW;
  vec_binding bnd_env = VEC_NEW;

  while (stack.len > 0) {
    NODE_IND_T ind = VEC_POP(&stack);
    parse_node node = tree.nodes.data[ind];
    switch (node.type) {
      case PT_LOWER_NAME:
      case PT_UPPER_NAME: {
        size_t ind = lookup_bnd_node(source, bnd_env, node);
        if (ind == bnd_env.len) {
          res.successful = false;
          tc_error err = {
            .err_type = BINDING_NOT_FOUND,
            .pos = ind,
          };
          VEC_PUSH(&res.errors, err);
        }
      }
      case PT_FN: {
        for (size_t i = 0; i < (node.sub_amt - 1) / 2; i++) {
          NODE_IND_T param = tree.inds.data[node.subs_start + i * 2];
          NODE_IND_T type = tree.inds.data[node.subs_start + i * 2 + 1];
          mk_bnd(&bnd_env, &type_env, param, type);
        }
      }
      case PT_CALL: {
        for (size_t i = 0; i < node.sub_amt; i++) {
          VEC_PUSH(&stack, tree.inds.data[node.subs_start + i]);
        }
      }
    }
  }

  return res;
}
