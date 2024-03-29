// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdbool.h>

#include "bitset.h"
#include "test.h"
#include "tests.h"
#include "vec.h"

void test_bitset(test_state *state) {
  test_group_start(state, "Bitset");

  {
    test_start(state, "Resize");
    bitset bs = bs_new();
    bs_resize(&bs, 3);
    test_assert_eq(state, bs.len, 0);
    test_assert_eq(state, bs.cap, 1);
    bs_resize(&bs, 8);
    test_assert_eq(state, bs.len, 0);
    test_assert_eq(state, bs.cap, 1);
    bs_resize(&bs, 9);
    test_assert_eq(state, bs.len, 0);
    test_assert_eq(state, bs.cap, 2);
    bs_free(&bs);
    test_end(state);
  }

  {
    test_start(state, "Free");
    bitset bs = bs_new();
    bs_resize(&bs, 2);
    bs_free(&bs);
    test_assert_eq(state, bs.data, NULL);
    test_end(state);
  }

  test_group_start(state, "Grow");
  {
    test_start(state, "When needs resize");

    bitset bs = bs_new();
    bs_resize(&bs, 1);
    test_assert_eq(state, bs.len, 0);
    test_assert_eq(state, bs.cap, 1);
    bs_grow(&bs, 9);
    test_assert_eq(state, bs.len, 0);
    test_assert_eq(state, bs.cap, BITSET_INITIAL_SIZE);
    bs_free(&bs);

    test_end(state);
  }

  {
    test_start(state, "Doesn't shrink");

    bitset bs = bs_new();
    bs_resize(&bs, 9);
    test_assert_eq(state, bs.len, 0);
    test_assert_eq(state, bs.cap, 2);
    bs_grow(&bs, 3);
    test_assert_eq(state, bs.len, 0);
    test_assert_eq(state, bs.cap, 2);
    bs_free(&bs);

    test_end(state);
  }
  test_group_end(state);

  test_group_start(state, "Push");
  {
    test_start(state, "When too small");

    bitset bs = bs_new();
    test_assert_eq(state, bs.cap, 0);
    bs_push(&bs, true);
    test_assert_eq(state, bs.len, 1);
    test_assert_eq(state, bs.cap, BITSET_INITIAL_SIZE);
    bs_push(&bs, false);
    test_assert_eq(state, bs.len, 2);
    test_assert_eq(state, bs.cap, BITSET_INITIAL_SIZE);
    test_assert_eq(state, bs_get(bs, 0), true);
    test_assert_eq(state, bs_get(bs, 1), false);
    bs_free(&bs);

    test_end(state);
  }

  {
    test_start(state, "With enough space");

    bitset bs = bs_new();
    bs_resize(&bs, 10);
    bs_push(&bs, false);
    test_assert_eq(state, bs.len, 1);
    bs_push(&bs, true);
    test_assert_eq(state, bs.len, 2);
    test_assert_eq(state, bs_get(bs, 0), false);
    test_assert_eq(state, bs_get(bs, 1), true);
    bs_free(&bs);

    test_end(state);
  }
  test_group_end(state);

  {
    test_start(state, "clear");

    const int n = 1000;
    bitset bs = bs_new();
    for (int i = 0; i < n; i++) {
      bs_push_true(&bs);
    }
    for (int i = 0; i < 8; i++) {
      bs_clear(bs, i * 8 + i);
      bs_clear(bs, i * 8 + i + 1);
    }
    int res = 0;
    for (int i = 0; i < n; i++) {
      res += bs_get(bs, i);
    }
    const int exp = n - 8 * 2;
    if (res != exp) {
      failf(state, "Expected %d, found %d", exp, res);
    }
    bs_free(&bs);

    test_end(state);
  }

  {
    test_start(state, "set");

    const int n = 100;
    bitset bs = bs_new();
    for (int i = 0; i < n; i++) {
      bs_push_false(&bs);
    }
    for (int i = 0; i < 8; i++) {
      bs_set(bs, i * 8 + i);
      bs_set(bs, i * 8 + i + 1);
    }
    int res = 0;
    for (int i = 0; i < n; i++) {
      res += bs_get(bs, i);
    }
    const int exp = 8 * 2;
    if (res != exp) {
      failf(state, "Expected %d, found %d", exp, res);
    }
    bs_free(&bs);

    test_end(state);
  }

  test_group_end(state);
}
