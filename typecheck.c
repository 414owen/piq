#include "typecheck.h"

typedef enum {
  POP_VAR,
  TC_NODE,
} tc_action;

tc_res typecheck(parse_tree tree) {
  tc_res res = {.successful = true, .types = VEC_NEW, .errors = VEC_NEW};
  vec_node_ind stack = VEC_NEW;
  VEC_PUSH(&stack, tree.root_ind);
  VEC_REPLICATE(&res.types, T_UNKNOWN, tree.nodes.len);

  while (stack.len > 0) {
    NODE_IND_T ind = VEC_POP(&stack);
    parse_node node = tree.nodes.data[ind];
    switch (node.type) {
      case PT_UPPER_NAME: {
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
