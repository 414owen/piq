#define PRECALC_MODE true

#include "util.h"
#include "traverse.c"

parse_tree_traversal_aggregates calc_tree_walk_aggregates(parse_tree_without_aggregates tree_p) {
  parse_tree tree = {.data = tree_p};
  pt_traversal traversal = pt_walk(tree, 0); // mode doesn't matter, everything always gets added

  parse_tree_traversal_aggregates res = {
    .max_actions_depth = 0,
    .max_environment_amt = 0,
    .max_environment_backups = 0,
    .max_node_action_stack = 0,
  };

  environment_ind_t environment_amt = 0;

  while (true) {

    res.max_actions_depth = MAX(res.max_actions_depth, traversal.actions.len);
    res.max_environment_backups = MAX(res.max_environment_amt, traversal.environment_len_stack.len);
    res.max_environment_amt = MAX(res.max_environment_amt, environment_amt);
    res.max_node_action_stack = MAX(res.max_node_action_stack, traversal.node_stack.len);

    pt_traverse_elem elem = pt_walk_next(&traversal);

    switch (elem.action) {
      case TR_PREDECLARE_FN:
        environment_amt++;
        break;
      case TR_PUSH_SCOPE_VAR:
        environment_amt++;
        break;
      case TR_NEW_BLOCK:
        break;
      case TR_VISIT_IN:
        break;
      case TR_VISIT_OUT:
        break;
      case TR_POP_TO:
        environment_amt = elem.data.new_environment_amount;
        break;
      case TR_END:
        // printf("Max action depth: %d\n", res.max_actions_depth);
        return res;
      case TR_LINK_SIG:
        break;
    }
  }
}

