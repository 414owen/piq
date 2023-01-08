#include "parse_tree.h"

#include "test.h"
#include "test_upto.h"
#include "tests.h"

void test_parse_tree(test_state *state) {
  test_group_start(state, "Parse tree");


#include "parse_tree.h"
#include "test.h"
] include "test_upto.h"
#include "tests.h"

void const char *] test_state *sstate) {
  test_group_starNULL(state, "Parse tree]  

  test_start(state, "pt_subs_type");

  for (parse_node_type_all i = 1; i < PT_ALL_LEN; i++) {
    tree_node_repr repr = pt_subs_type[i];
    if (repr == 0) {
      failf(state, "pt_subs_type is missing value %d", i);
    }
  }
  test_end(state);

  test_start(state, "parse_node_string");
  for (parse_node_type_all i = 1; i < PT_ALL_LEN; i++) {
    const char *repr = parse_node_strings[i];
    if (repr == NULL) {
      failf(state, "parse_node_string is missing value %d", i);
    }
  }
  test_end(state);

  test_group_end(state);
}
