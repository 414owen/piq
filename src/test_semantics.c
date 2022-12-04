#include "test.h"
#include "typedefs.h"

// This module is for making sure pieces of code/logic do what I think they do

// result should always be positive

typedef i32 (*mod_fn)(i32, i32);

i32 mod_trunc(i32 a, i32 b) { return a % b; }

i32 mod_floor(i32 a, i32 b) { return (a % b + b) % b; }

i32 mod_euc(i32 a, i32 b) {
  i32 r = a % b;
  if (r >= 0)
    return r;
  if (b > 0)
    return r + b;
  return r - b;
}

typedef struct {
  char *name;
  i32 dividend;
  i32 divisor;
  i32 expected_trunc;
  i32 expected_euc;
  i32 expected_floor;
} test_case;

#define mkcase(a, b) .dividend = a, .divisor = b, .name = #a " mod " #b

test_case cases[] = {
  {
    mkcase(5, 3),
    .expected_trunc = 2,
    .expected_euc = 2,
    .expected_floor = 2,
  },
  {
    mkcase(5, -3),
    .expected_trunc = 2,
    .expected_euc = 2,
    .expected_floor = -1,
  },
  {
    mkcase(-5, 3),
    .expected_trunc = -2,
    .expected_euc = 1,
    .expected_floor = 1,
  },
  {
    mkcase(-5, -3),
    .expected_trunc = -2,
    .expected_euc = 1,
    .expected_floor = -2,
  },
};

#undef mkcase

void run_modulo_test(test_state *state, char *name, mod_fn fn, i32 dividend,
                     i32 divisor, i32 expected) {
  test_start(state, name);
  i32 res = fn(dividend, divisor);
  if (res != expected) {
    failf(state, "Expected: %d, Got: %d", expected, res);
  }
  test_end(state);
}

void test_modulo(test_state *state) {
  test_group_start(state, "Truncated");
  for (unsigned i = 0; i < STATIC_LEN(cases); i++) {
    test_case c = cases[i];
    run_modulo_test(
      state, c.name, mod_trunc, c.dividend, c.divisor, c.expected_trunc);
  }
  test_group_end(state);
  test_group_start(state, "Floored");
  for (unsigned i = 0; i < STATIC_LEN(cases); i++) {
    test_case c = cases[i];
    run_modulo_test(
      state, c.name, mod_floor, c.dividend, c.divisor, c.expected_floor);
  }
  test_group_end(state);
  test_group_start(state, "Euclidean");
  for (unsigned i = 0; i < STATIC_LEN(cases); i++) {
    test_case c = cases[i];
    run_modulo_test(
      state, c.name, mod_euc, c.dividend, c.divisor, c.expected_euc);
  }
  test_group_end(state);
}

void test_semantics(test_state *state) {
  test_group_start(state, "Semantics");
  test_modulo(state);
  test_group_end(state);
}
