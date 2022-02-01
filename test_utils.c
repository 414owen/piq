#include "test.h"
#include "util.h"

static char *sep = " . ";

void test_memclone(test_state *state) {
  test_start(state, "memclone");
  static int arr[] = {1, 2, 3, 4, 5};
  int *res = memclone(arr, STATIC_LEN(arr) * sizeof(arr[0]));
  for (int i = 0; i < STATIC_LEN(arr); i++) {
    if (res[i] != arr[i])
      failf(state, "%d != %d", res[i], arr[i]);
  }
  free(res);
  test_end(state);
}

void test_join(test_state *state, char **strs, size_t amt, char *exp) {
  char *join_res = join(amt, strs, sep);
  if (strcmp(join_res, exp) != 0)
    test_fail_eq(state, join_res, exp);
  free(join_res);
}

void run_join_tests(test_state *state) {
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
}

void test_asprintf(test_state *state) {
  test_start(state, "asprintf");
  char *res;
  static const char *fmt = "Hello World! %d";
  asprintf(&res, fmt, 42);
  static const char *exp = "Hello World! 42";
  if (strcmp(exp, res) != 0)
    failf(state, "'%s' didn't format to '%s'", fmt, exp);
  free(res);
  test_end(state);
}

void test_utils(test_state *state) {
  test_group_start(state, "Utils");
  run_join_tests(state);
  test_memclone(state);
  test_asprintf(state);
  test_group_end(state);
}
