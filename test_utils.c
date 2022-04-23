#include <sys/stat.h>
#include <unistd.h>

#include "diagnostic.h"
#include "test.h"
#include "util.h"
#include "token.h"

static char *sep = " . ";

static void test_memclone(test_state *state) {
  test_start(state, "memclone");
  static int arr[] = {1, 2, 3, 4, 5};
  int *res = memclone(arr, STATIC_LEN(arr) * sizeof(arr[0]));
  for (size_t i = 0; i < STATIC_LEN(arr); i++) {
    if (res[i] != arr[i])
      failf(state, "%d != %d", res[i], arr[i]);
  }
  free(res);
  test_end(state);
}

static void test_join(test_state *state, const char *const *strs, size_t amt,
                      char *exp) {
  char *join_res = join(amt, strs, sep);
  if (strcmp(join_res, exp) != 0)
    test_fail_eq(state, join_res, exp);
  free(join_res);
}

static void run_join_tests(test_state *state) {
  test_group_start(state, "Join");
  {
    test_start(state, "Empty");
    static const char *strs[] = {};
    test_join(state, strs, STATIC_LEN(strs), "");
    test_end(state);
  }
  {
    test_start(state, "One");
    static const char *strs[] = {"hi"};
    test_join(state, strs, STATIC_LEN(strs), "hi");
    test_end(state);
  }
  {
    test_start(state, "Two");
    static const char *strs[] = {"hi", "yes"};
    test_join(state, strs, STATIC_LEN(strs), "hi . yes");
    test_end(state);
  }
  {
    test_start(state, "Three");
    static const char *strs[] = {"hi", "yes", "wow"};
    test_join(state, strs, STATIC_LEN(strs), "hi . yes . wow");
    test_end(state);
  }
  test_group_end(state);
}

static void test_timespec_subtract(test_state *state) {
  test_group_start(state, "Timespec");

  struct timespec res;
  struct timespec x;
  struct timespec y;

  {
    test_start(state, "subtract pos");
    timespec_get(&x, TIME_UTC);
    y = x;
    x.tv_sec++;
    int neg = timespec_subtract(&res, &x, &y);
    test_assert_eq(state, neg, 0);
    test_assert_eq(state, res.tv_sec, 1);
    test_end(state);
  }

  {
    test_start(state, "subtract neg");
    timespec_get(&x, TIME_UTC);
    y = x;
    x.tv_sec++;
    int neg = timespec_subtract(&res, &y, &x);
    test_assert_eq(state, neg, 1);
    test_assert_eq(state, res.tv_sec, -1);
    test_end(state);
  }

  {
    test_start(state, "carries nsec");
    timespec_get(&x, TIME_UTC);
    y = x;
    x.tv_nsec++;
    int neg = timespec_subtract(&res, &y, &x);
    test_assert_eq(state, neg, 1);
    test_assert_eq(state, res.tv_sec, -1);
    test_end(state);
  }

  {
    test_start(state, "overflows nsecs");
    timespec_get(&x, TIME_UTC);
    y = x;
    x.tv_nsec = 1 + 1000000000 + y.tv_nsec;
    int neg = timespec_subtract(&res, &x, &y);
    test_assert_eq(state, neg, 0);
    test_assert_eq(state, res.tv_sec, 1);
    test_end(state);
  }

  test_group_end(state);
}

static void test_asprintf(test_state *state) {
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

static void test_mkdirp(test_state *state) {
  test_group_start(state, "mkdir -p");

#ifndef _WIN32
  {
    char *path = strdup("/tmp/lang-c/test/folder/pls/create");
    mkdirp(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (!directory_exists(path)) {
      failf(state, "Directory didn't exist");
    }
    rm_r("/tmp/lang-c");
    free(path);
  }
#endif

  test_group_end(state);
}

void test_utils(test_state *state) {
  test_group_start(state, "Utils");
  run_join_tests(state);
  test_memclone(state);
  test_asprintf(state);
  test_timespec_subtract(state);
  test_mkdirp(state);
  test_group_end(state);
}