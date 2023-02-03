#include "parse_tree.h"

#include "test.h"
#include "test_upto.h"
#include "tests.h"

void test_parse_tree(test_state *state) {
  test_group_start(state, "Parse tree");

  test_start(state, "pt_subs_types");
  // Yes, this is meant to be 64. We use PT_ALL_LEN as a bitset index.
  if (PT_ALL_LEN >= 65) {
    failf(state, "Too many parse node types. traverse.c uses a uint64_t as a note type bitset.");
  }
  test_end(state);

  test_start(state, "pt_subs_types");
  for (parse_node_type_all i = 1; i < PT_ALL_LEN; i++) {
    tree_node_repr repr = pt_subs_type[i];
    if (repr == 0) {
      failf(state, "pt_subs_types is missing value %d", i);
    }
  }
  test_end(state);

  test_start(state, "parse_node_strings");
  for (parse_node_type_all i = 1; i < PT_ALL_LEN; i++) {
    const char *repr = parse_node_strings[i];
    if (repr == NULL) {
      failf(state, "parse_node_strings is missing value %d", i);
    }
  }
  test_end(state);

  test_start(state, "parse_node_categories");
  for (parse_node_type_all i = 1; i < PT_ALL_LEN; i++) {
    parse_node_category cat = parse_node_categories[i];
    if (cat == 0) {
      failf(state, "parse_node_categoeirs is missing value %d", i);
    }
  }
  test_end(state);

  test_group_end(state);
}
