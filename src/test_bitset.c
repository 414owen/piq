#include <stdbool.h>

#include "bitset.h"
#include "test.h"
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
    test_assert_eq(state, bs.cap, VEC_FIRST_SIZE);
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
    test_assert_eq(state, bs.cap, VEC_FIRST_SIZE);
    bs_push(&bs, false);
    test_assert_eq(state, bs.len, 2);
    test_assert_eq(state, bs.cap, VEC_FIRST_SIZE);
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

  test_group_end(state);
}
