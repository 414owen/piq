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
      u32 res = ahm_lookup(&hm, &i, NULL);
      if (bs_get(hm.occupied, res)) {
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
      u32 res_ind = ahm_lookup(&hm, &i, NULL);
      u64 key_res = ((uint64_t *)hm.keys)[res_ind];
      u64 val_res = ((uint64_t *)hm.vals)[res_ind];
      if (!bs_get(hm.occupied, res_ind)) {
        failf(state, "Expected value %llu", i);
      } else if (key_res != i) {
        failf(state, "Wrong key %llu: %llu", i, key_res);
      } else if (val_res != i + 1) {
        failf(state, "Wrong value %llu: %llu", i, val_res);
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
      u32 res_ind = ahm_lookup(&hm, &i, NULL);
      u64 key_res = ((uint64_t *)hm.keys)[res_ind];
      u64 val_res = ((uint64_t *)hm.vals)[res_ind];
      if (!bs_get(hm.occupied, res_ind)) {
        failf(state, "Expected value %llu", i);
      } else if (key_res != i) {
        failf(state, "Wrong key %llu: %llu", i, key_res);
      } else if (val_res != i + n) {
        failf(state, "Wrong value %llu: %llu", i, val_res);
      }
    }
    ahm_free(&hm);
    test_end(state);
  }

  {
    test_start(state, "hashset");
    a_hashmap hm = hashset_new(u64, cmp_u64, hash_u64, hash_u64);
    for (u64 i = 0; i < 1000; i++) {
      ahm_upsert(&hm, &i, &i, NULL, NULL);
    }
    for (u64 i = 0; i < 1000; i++) {
      char *s = format_to_string("%lu", i);
      free(s);
      u32 res_ind = ahm_lookup(&hm, &i, NULL);
      u64 key_res = ((uint64_t *)hm.keys)[res_ind];
      if (!bs_get(hm.occupied, res_ind)) {
        failf(state, "Expected value %llu", i);
      } else if (key_res != i) {
        failf(state, "Wrong key %llu: %llu", i, key_res);
      }
    }
    ahm_free(&hm);
    test_end(state);
  }

  {
    const int n = 50;
    test_start(state, "rehashing and deletion");
    a_hashmap hm =
      __ahm_new(2, sizeof(u64), sizeof(u64), cmp_u64, hash_u64, hash_u64);
    ahm_upsert(&hm, &n, &n, &n, NULL);
    {
      u32 ind = ahm_lookup(&hm, &n, NULL);
      if (!bs_get(hm.occupied, ind)) {
        failf(state, "Couldn't find key");
      }
    }
    u32 rm_ind = ahm_remove(&hm, &n, NULL);
    {
      u32 ind = ahm_lookup(&hm, &n, NULL);
      if (bs_get(hm.occupied, ind)) {
        failf(state, "Unexpected key");
      }
    }
    ahm_insert_at(&hm, rm_ind, &n, &n);
    {
      u32 ind = ahm_lookup(&hm, &n, NULL);
      if (!bs_get(hm.occupied, ind)) {
        failf(state, "Couldn't find key");
      }
    }
    ahm_free(&hm);
    test_end(state);
  }

  test_group_end(state);
}
