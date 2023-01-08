#pragma once

// We can store tree children inline if there are a fixed number less than tree
// for any given node type. This is applicable to parse_tree and type trees.
// Having functions that take a node_tag and return a tree_node_repr allows
// us to write functions that deal with all node types, without having to
// understand the node types directly.
typedef enum {
  SUBS_NONE = 1,
  SUBS_ONE = 2,
  SUBS_TWO = 3,
  SUBS_EXTERNAL = 4,
} tree_node_repr;
