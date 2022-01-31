#include "typecheck.h"

typedef struct {
  tc_res res;
  parse_tree tree;
} tc_state;

void tc_node(tc_state *s, NODE_IND_T ind) {
  if (s->res.types.data[ind] != T_UNKNOWN)
    return;
  parse_node n = s->tree.nodes.data[ind];
  switch (n.type) {
    case PT_CALL: {
      // tc_node();
    }
  }
}

tc_res typecheck(parse_tree tree) {
  tc_state state = {
    .res = {.successful = true, .types = VEC_NEW, .errors = VEC_NEW},
    .tree = tree,
  };
  VEC_REPLICATE(&state.res.types, T_UNKNOWN, tree.nodes.len);
  tc_node(&state, tree.root_ind);
  return state.res;
}
