#include "util.h"
#include "test.h"

static char *sep = " . ";

void test_join(test_state *state, char **strs, size_t amt, char *exp) {
  char *join_res = join(amt, strs, sep);
  if (strcmp(join_res, exp) != 0) test_fail_eq(state, join_res, exp);
  free(join_res);
}

void test_utils(test_state *state) {
  test_group_start(state, "Utils");

  test_group_start(state, "Join");
  {
    test_start(state, "Empty");
    static char *strs[] = {};
    test_join(state, strs, STATIC_LEN(strs), "");
    test_end(state);
  }
  {
    test_start(state, "One");
    static char *strs[] = {"hi"};
    test_join(state, strs, STATIC_LEN(strs), "hi");
    test_end(state);
  }
  {
    test_start(state, "Two");
    static char *strs[] = {"hi", "yes"};
    test_join(state, strs, STATIC_LEN(strs), "hi . yes");
    test_end(state);
  }
  {
    test_start(state, "Three");
    static char *strs[] = {"hi", "yes", "wow"};
    test_join(state, strs, STATIC_LEN(strs), "hi . yes . wow");
    test_end(state);
  }
  test_group_end(state);

  test_group_end(state);
}
