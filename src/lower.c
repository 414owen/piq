#include "ir.h"
#include "parse_tree.h"

typedef struct {
  enum { LOWER_NODE } tag;
  node_ind_t ind;
} action;

VEC_DECL(action);

typedef struct {
  vec_action actions;
} state;

static void push_lower_node(state *state, node_ind_t node_ind) {
  action a = {
    .tag = LOWER_NODE,
    .ind = node_ind,
  };
}

ir_module lower(parse_tree tree) {
  ir_module res;
  state state = {
    .actions = VEC_NEW,
  };
  push_lower_node(&state, tree.root_ind);
  return res;
}
