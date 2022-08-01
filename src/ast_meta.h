#pragma once

// We can store tree children inline if there are a fixed number less than tree
// for any given node type. This is applicable to parse_tree and type trees.
// Having functions that take a node_tag and return a tree_node_repr allows
// us to write functions that deal with all node types, without having to
// understand the node types directly.
typedef enum {
  SUBS_EXTERNAL = -1,
  SUBS_NONE = 0,
  SUBS_ONE = 1,
  SUBS_TWO = 2,
} tree_node_repr;
