#include "test.h"
#include "vec.h"

static const char *str = "hi";

void test_vec(test_state *state) {
  test_group_start(state, "Vec");

  {
    test_start(state, "New");
    vec_string v = {.cap = 2, .data = (char **)state, .len = 2};
    v = (vec_string)VEC_NEW;
    test_assert_eq(state, v.len, 0);
    test_assert_eq(state, v.cap, 0);
    test_assert_eq(state, v.data, NULL);
    VEC_FREE(&v);
    test_end(state);
  }

  {
    test_start(state, "Resize");
    vec_string v = VEC_NEW;
    VEC_RESIZE(&v, 2);
    test_assert_eq(state, v.len, 0);
    test_assert_eq(state, v.cap, 2);
    VEC_FREE(&v);
    test_end(state);
  }

  {
    test_start(state, "FREE");
    vec_string v = VEC_NEW;
    VEC_RESIZE(&v, 2);
    VEC_FREE(&v);
    test_assert_eq(state, v.data, NULL);
    test_end(state);
  }

  test_group_start(state, "Grow");
  {
    test_start(state, "When needs resize");

    vec_string v = VEC_NEW;
    VEC_RESIZE(&v, 1);
    test_assert_eq(state, v.len, 0);
    test_assert_eq(state, v.cap, 1);
    VEC_GROW(&v, 3);
    test_assert_eq(state, v.len, 0);
    test_assert_eq(state, v.cap, VEC_FIRST_SIZE);
    VEC_FREE(&v);

    test_end(state);
  }

  {
    test_start(state, "Doesn't shrink");

    vec_string v = VEC_NEW;
    VEC_RESIZE(&v, 5);
    test_assert_eq(state, v.len, 0);
    test_assert_eq(state, v.cap, 5);
    VEC_GROW(&v, 3);
    test_assert_eq(state, v.len, 0);
    test_assert_eq(state, v.cap, 5);
    VEC_FREE(&v);

    test_end(state);
  }
  test_group_end(state);

  test_group_start(state, "Push");
  {
    test_start(state, "When too small");

    vec_string v = VEC_NEW;
    VEC_PUSH(&v, str);
    test_assert_eq(state, v.len, 1);
    VEC_PUSH(&v, str);
    test_assert_eq(state, v.len, 2);
    VEC_FREE(&v);

    test_end(state);
  }

  {
    test_start(state, "With enough space");

    vec_string v = VEC_NEW;
    VEC_RESIZE(&v, 10);
    VEC_PUSH(&v, str);
    test_assert_eq(state, v.len, 1);
    VEC_PUSH(&v, str);
    test_assert_eq(state, v.len, 2);
    test_assert_eq(state, strcmp(v.data[0], str), 0);
    test_assert_eq(state, strcmp(v.data[1], str), 0);
    VEC_FREE(&v);

    test_end(state);
  }
  test_group_end(state);

  {
    test_start(state, "Append");

    vec_string v = VEC_NEW;
    VEC_PUSH(&v, str);
    VEC_PUSH(&v, str);
    static const char *strs[20];
    for (int i = 0; i < STATIC_LEN(strs); i++)
      strs[i] = str;
    VEC_APPEND(&v, STATIC_LEN(strs), strs);
    test_assert_eq(state, v.len, STATIC_LEN(strs) + 2);
    test_assert(state, v.cap >= v.len);
    for (int i = 0; i < STATIC_LEN(strs) + 2; i++)
      test_assert_eq(state, strcmp(v.data[i], str), 0);
    VEC_FREE(&v);

    test_end(state);
  }

  {
    test_start(state, "Replicate");

    vec_string v = VEC_NEW;
    VEC_PUSH(&v, str);
    VEC_PUSH(&v, str);
    VEC_REPLICATE(&v, 100, str);
    test_assert_eq(state, v.len, 102);
    test_assert(state, v.cap >= v.len);
    for (int i = 0; i < 102; i++)
      test_assert_eq(state, strcmp(v.data[i], str), 0);
    VEC_FREE(&v);

    test_end(state);
  }

  test_group_end(state);
}
