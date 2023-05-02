#include <stdint.h>
#include <stdint.h>

#include "consts.h"
#include "calc_tree_aggregates.h"
#include "parse_tree.h"
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

parse_tree_aggregates calculate_tree_aggregates(parse_tree_without_aggregates tree) {
  parse_tree_traversal_aggregates tr_aggs = calc_tree_walk_aggregates(tree);
  parse_tree_aggregates res = {
    .max_depth = calc_max_depth(tree),
    .max_actions_depth = tr_aggs.max_actions_depth,
    .max_environment_amt = tr_aggs.max_environment_amt,
    .max_environment_backups = tr_aggs.max_environment_backups,
    .max_node_action_stack = tr_aggs.max_node_action_stack,
  };
  return res;
}
