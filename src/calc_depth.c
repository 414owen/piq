#include "consts.h"
#include "calc_depth.h"
#include "parse_tree.h"
#include "vec.h"
#include "util.h"

typedef struct {
  node_ind_t node_index;
  node_ind_t depth;
} depth_state;

VEC_DECL(depth_state);

node_ind_t calculate_tree_depth(parse_tree tree) {
  node_ind_t total = 0;
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
    total = MAX(total, elem.depth);
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
  return total;
}
