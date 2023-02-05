#include "traverse.h"
#include "test.h"

static const char *action_names[] = {
  #define mk_map(a) [a] = #a
  mk_map(TR_PUSH_SCOPE_VAR),
  mk_map(TR_NEW_BLOCK),
  mk_map(TR_VISIT_IN),
  mk_map(TR_VISIT_OUT),
  mk_map(TR_POP_TO),
  mk_map(TR_END),
  mk_map(TR_LINK_SIG),
  #undef mk_map
};

static bool test_elems_match(test_state *state, pt_traverse_elem *elems, int amount, pt_traversal *traversal) {
  for (int i = 0; i < amount; i++) {
    pt_traverse_elem a = elems[i];
    pt_traverse_elem b = pt_scoped_traverse_next(traversal);
    if (b.action == TR_END) {
      failf(state, "Not enough traversal items. Expected %d.", amount);
    }
    if (a.action != b.action) {
      failf(state, "Traversal item mismatch. Expected '%s', got '%s'.", action_name(a.action), action_name(b.action));
    }
  }
  pt_traverse_elem b = pt_scoped_traverse_next(traversal);
  if (b.action != TR_END) {
    failf(state, "Too many traversal items. Expected %d.", amount);
  }
}

void test_traverse(test_state *state) {
  test_group_start(state, "AST Traversal");
  
  test_group_end(state);
}
