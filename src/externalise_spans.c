#include "parse_tree.h"

// TODO this can be run in parallel with name resolution

void externalise_spans(parse_tree *tree) {
  span *const spans = malloc(sizeof(span) * tree->node_amt);
  const node_ind_t node_amt = tree->node_amt;
  parse_node *const nodes = tree->nodes;
  for (node_ind_t i = 0; i < node_amt; i++) {
    // At first this also wrote zeros into the node's span,
    // but that causes this function to slow down by about
    // 33%, so I guess we'll just have to deal.
    spans[i] = nodes[i].data.span;
  }
  tree->spans = spans;
}
