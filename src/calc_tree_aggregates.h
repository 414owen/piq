#pragma once

#include "consts.h"
#include "parse_tree.h"

typedef struct {
  node_ind_t max_actions_depth;
  node_ind_t max_environment_amt;
  node_ind_t max_environment_backups;
} parse_tree_traversal_aggregates;

parse_tree_traversal_aggregates calc_tree_walk_aggregates(parse_tree_without_aggregates tree_p);
parse_tree_aggregates calculate_tree_aggregates(parse_tree_without_aggregates tree);
