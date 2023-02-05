#include "test_upto.h"
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

typedef struct {
  scoped_traverse_action action;
  union {
    struct {
      node_ind_t ind;
      parse_node_type_all type;
    } node;
    uint32_t amount;
  };
} test_traverse_elem;

static void test_node_elems_match(test_state *state, pt_traverse_elem a, test_traverse_elem b) {
  test_assert_eq_fmt_a(state, a.data.node_data.node.type.all, b.node.type, "%s", parse_node_strings);
  test_assert_eq_fmt(state, a.data.node_data.node_index, b.node.ind, "%d");
}

static bool test_elems_match(test_state *state, pt_traversal *traversal, test_traverse_elem *elems, int amount) {
  for (int i = 0; i < amount; i++) {
    pt_traverse_elem a = pt_scoped_traverse_next(traversal);
    test_traverse_elem b = elems[i];
    if (b.action == TR_END) {
      failf(state, "Not enough traversal items. Expected %d.", amount);
    }
    if (a.action != b.action) {
      failf(state, "Traversal item mismatch. Expected '%s', got '%s'.", action_name(a.action), action_name(b.action));
    }
    switch (a.action) {
      case TR_PUSH_SCOPE_VAR:
      case TR_VISIT_IN:
      case TR_VISIT_OUT:
      case TR_NEW_BLOCK:
        test_node_elems_match(state, a, b);
        break;
      case TR_POP_TO:
        break;
      case TR_END:
        break;
      case TR_LINK_SIG:
        break;
    }
  }
  pt_traverse_elem b = pt_scoped_traverse_next(traversal);
  if (b.action != TR_END) {
    failf(state, "Too many traversal items. Expected %d.", amount);
  }
}

static void test_traversal(test_state *state, const char *input, traverse_mode mode, pt_traverse_elem *elems, int amount) {
  const parse_tree_res tres = test_upto_parse_tree(state, input);
  if (!tres.succeeded) {
    return;
  }
  pt_traversal traversal = pt_scoped_traverse(tres.tree, mode);
  test_elems_match(state, &traversal, elems, amount);
}

static const char *input =
  "(sig a (Fn ((i8, i8)) i8))\n"
  "(fun a ((b, c)) (add ((fn () 2) ()) 3))";
    
static test_traverse_elem print_mode_elems[] = {
  {
    .action = TR_VISIT_IN,
    .node_ind = 
  },
};

void test_traverse(test_state *state) {
  test_group_start(state, "AST Traversal");


  {
    test_start(state, "Print tree mode");
    test_traversal(state, input, )
    test_end(state);
  }

  test_group_end(state);
}
