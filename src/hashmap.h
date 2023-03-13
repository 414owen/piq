#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include "bitset.h"
#include "typedefs.h"

#define HASH_TYPE uint32_t

// Keep it a power of 2
#define N_BUCKETS_START 512

typedef bool (*eq_cmp)(const void *, const void *, const void *);
typedef uint32_t (*hasher)(const void *, const void *);

typedef struct {
  uint32_t n_buckets;
  uint32_t n_elems;
  uint32_t mask;
  const uint32_t keysize;
  const uint32_t valsize;
  const eq_cmp compare_newkey;
  const hasher hash_newkey;
  const hasher hash_storedkey;
  char *restrict keys;
  char *restrict vals;
  bitset occupied;
  bitset tombstoned;
} a_hashmap;

#define ahm_new(keytype, valtype, ...)                                         \
  __ahm_new(N_BUCKETS_START, sizeof(keytype), sizeof(valtype), __VA_ARGS__)

#define hashset_new(keytype, ...)                                         \
  __ahm_new(N_BUCKETS_START, sizeof(keytype), 0, __VA_ARGS__)

a_hashmap __ahm_new(uint32_t n_buckets, uint32_t keysize, uint32_t valsize,
                    eq_cmp cmp_newkey, hasher hash_newkey,
                    hasher hash_storedkey);

u32 ahm_lookup(a_hashmap *hm, const void *key, void *context);
u32 ahm_remove(a_hashmap *hm, const void *key, void *context);
u32 ahm_remove_stored(a_hashmap *hm, const void *key, void *context);
bool ahm_upsert(a_hashmap *hm, const void *key, const void *key_stored,
                const void *val, void *context);
void ahm_insert_at(a_hashmap *hm, u32 index, const void *key_stored, const void *val);

// Don't use a proto-key, use a real key
void ahm_insert_stored(a_hashmap *hm, const void *key_stored, const void *val,
                       void *context);
void ahm_free(a_hashmap *hm);
