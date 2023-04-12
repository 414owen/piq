// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "diagnostic.h"
#include "test.h"
#include "tests.h"
#include "timespec.h"
#include "timing.h"
#include "token.h"
#include "util.h"

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
    test_join(state, NULL, 0, "");
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
#ifdef STD_C11
  test_group_start(state, "Timespec");

  timespec res;
  timespec x;
  timespec y;

  {
    test_start(state, "subtract pos");
    timespec_get(&x, TIME_UTC);
    y = x;
    x.tv_sec++;
    res = timespec_subtract(x, y);
    test_assert(state, !timespec_negative(res));
    test_assert_eq(state, res.tv_sec, 1);
    test_assert_eq(state, res.tv_nsec, 0);
    test_end(state);
  }

  {
    test_start(state, "subtract neg");
    timespec_get(&x, TIME_UTC);
    y = x;
    x.tv_sec++;
    res = timespec_subtract(y, x);
    test_assert(state, timespec_negative(res));
    test_assert_eq(state, res.tv_sec, -1);
    test_assert_eq(state, res.tv_nsec, 0);
    test_end(state);
  }

  {
    test_start(state, "carries nsec");
    timespec_get(&x, TIME_UTC);
    y = x;
    y.tv_nsec++;
    res = timespec_subtract(x, y);
    test_assert(state, timespec_negative(res));
    // negative nanoseconds is invalid according to POSIX
    test_assert_eq(state, res.tv_sec, -1);
    test_assert_eq(state, res.tv_nsec, 1e9 - 1);
    test_end(state);
  }

  test_group_end(state);
#endif
}

static void test_monotonic_time(test_state *state) {
  test_start(state, "subsequent calls lead to greater times");
  timespec x = get_monotonic_time();
  timespec y = get_monotonic_time();
  test_assert(state, timespec_gt(y, x));
  test_end(state);
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
  test_start(state, "mkdir -p");

  {
    char *path = strdup("/tmp/lang-c/test/folder/pls/create");
    mkdirp(path, S_IRWXU | S_IRWXG | S_IROTH);
    if (!directory_exists(path)) {
      failf(state, "Directory didn't exist");
    }
    rm_r("/tmp/lang-c");
    free(path);
  }

  test_end(state);
}

void test_utils(test_state *state) {
  test_group_start(state, "Utils");
  run_join_tests(state);
  test_memclone(state);
  test_asprintf(state);
  test_timespec_subtract(state);
  test_monotonic_time(state);
  test_mkdirp(state);
  test_group_end(state);
}
