#include <stdint.h>

#include "hashmap.h"
#include "test.h"
#include "typedefs.h"

// These are copied here so that we don't need to compile
// test hashers, or use CPP
static uint32_t rotate_left(uint32_t n, uint32_t d) {
  return (n << d) | (n >> (32 - d));
}

// hash function based on rustc's
static uint32_t hash_eight_bytes(uint32_t seed, uint64_t bytes) {
  return (rotate_left(seed, 5) ^ bytes) * 0x9e3779b9;
}

static uint32_t hash_u64(const void *val, const void *context) {
  return hash_eight_bytes(0, *((uint64_t *)val));
}

static bool cmp_u64(const void *a, const void *b, const void *context) {
  return *((uint64_t *)a) == *((uint64_t *)b);
}

a_hashmap mk_hm(void) { return ahm_new(u64, u64, cmp_u64, hash_u64, hash_u64); }

void test_hashmap(test_state *state) {
  test_group_start(state, "HashMap");

  {
    test_start(state, "empty");
    a_hashmap hm = mk_hm();
    for (u64 i = 0; i < 1000; i++) {
      if (ahm_lookup(&hm, &i, NULL) != NULL) {
        failf(state, "Expected hashmap to be empty!");
      }
    }
    ahm_free(&hm);
    test_end(state);
  }

  {
    test_start(state, "reproduces");
    a_hashmap hm = mk_hm();
    for (u64 i = 0; i < 1000; i++) {
      u64 val = i + 1;
      ahm_upsert(&hm, &i, &i, &val, NULL);
    }
    for (u64 i = 0; i < 1000; i++) {
      char *s = format_to_string("%lu", i);
      free(s);
      u64 *res = ahm_lookup(&hm, &i, NULL);
      if (res == NULL) {
        failf(state, "Expected value %llu", i);
      } else if (*res != i + 1) {
        failf(state, "Wrong value %llu: %llu", i, *res);
      }
    }
    ahm_free(&hm);
    test_end(state);
  }

  {
    const int n = 100;
    test_start(state, "overwrites");
    a_hashmap hm = mk_hm();
    for (int j = 1; j <= n; j++) {
      for (u64 i = 0; i < 1000; i++) {
        u64 val = i + j;
        ahm_upsert(&hm, &i, &i, &val, NULL);
      }
    }
    for (u64 i = 0; i < 1000; i++) {
      char *s = format_to_string("%lu", i);
      free(s);
      u64 *res = ahm_lookup(&hm, &i, NULL);
      if (res == NULL) {
        failf(state, "Expected value %llu", i);
      } else if (*res != i + n) {
        failf(state, "Wrong value %llu: %llu", i, *res);
      }
    }
    ahm_free(&hm);
    test_end(state);
  }

  test_group_end(state);
}
