#include <stdint.h>
#include <stdint.h>

#include "consts.h"
#include "calc_tree_aggregates.h"
#include "parse_tree.h"
#include "traversal.h"
#include "vec.h"

typedef struct {
  node_ind_t node_index;
  node_ind_t depth;
} depth_state;

VEC_DECL(depth_state);

static node_ind_t calc_max_depth(parse_tree_without_aggregates tree) {
  node_ind_t max_depth = 0;
  vec_depth_state stack = VEC_NEW;
  for (node_ind_t i = 0; i < tree.root_subs_amt; i++) {
    depth_state initial = {
      .node_index = tree.inds[tree.root_subs_start + i],
      .depth = 1,
    };
    VEC_PUSH(&stack, initial);
  }

  while (stack.len > 0) {
    depth_state elem;
    VEC_POP(&stack, &elem);
    max_depth = MAX(elem.depth, max_depth);
    parse_node n = tree.nodes[elem.node_index];
    elem.depth++;

    switch (pt_subs_type[n.type.all]) {
      case SUBS_NONE:
        break;
      case SUBS_ONE:
        elem.node_index = n.data.one_sub.ind;
        VEC_PUSH(&stack, elem);
        break;
      case SUBS_TWO:
        elem.node_index = n.data.two_subs.a;
        VEC_PUSH(&stack, elem);
        elem.node_index = n.data.two_subs.b;
        VEC_PUSH(&stack, elem);
        break;
      case SUBS_EXTERNAL:
        for (node_ind_t i = 0; i < n.data.more_subs.amt; i++) {
          elem.node_index = tree.inds[n.data.more_subs.start + i];
          VEC_PUSH(&stack, elem);
        }
        break;
    }
  }
  return max_depth;
}

typedef struct {
  vec_traverse_action actions;
  vec_node_ind node_inds;
} max_action_state;

HEDLEY_INLINE
static void calc_max_actions_push_action(max_action_state *state, traverse_action_internal act, node_ind_t node_index) {
  VEC_PUSH(&state->actions, act);
  VEC_PUSH(&state->node_inds, node_index);
}



static void calc_max_actions_push_letrec(parse_node *nodes, node_ind_t *inds, max_action_state *state, node_ind_t start, node_ind_t amount) {

  for (node_ind_t i = 0; i < amount; i++) {
    const node_ind_t node_index = inds[start + amount - 1 - i];
    parse_node node = nodes[node_index];
    switch (node.type.statement) {
      case PT_STATEMENT_FUN: {
        // '0' is not a node index, just a scope amount
        calc_max_actions_push_action(state, TR_ACT_POP_SCOPE_TO, 0);
        calc_max_actions_push_external_initial(state, )
        tr_initial_external_sub_statement(traversal, data);
        tr_maybe_backup_scope(traversal);
        break;
      }
      default:
        tr_push_initial(traversal, node_index);
        break;
    }
  }
}

static node_ind_t calc_max_actions_depth(parse_tree_without_aggregates tree) {
  VEC_LEN_T max_actions;
  VEC_LEN_T max_node_inds;
  max_action_state state = {
    
  };
  return res;
}


parse_tree_aggregates calculate_tree_aggregates(parse_tree_without_aggregates tree) {
  parse_tree_aggregates res = {
    .max_depth = calc_max_depth(tree),
    .max_actions_depth = calc_max_actions_depth(tree),
  };
  return res;
}
