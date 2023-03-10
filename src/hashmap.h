#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#define HASH_TYPE uint32_t
#define AHM_VEC_LEN_T uint32_t

typedef bool (*eq_cmp)(const void *, const void *, const void *);
typedef uint32_t (*hasher)(const void *, const void *);

typedef struct {
  uint32_t len;
  uint32_t cap;
  void *keys;
  void *vals;
} ahm_keys_and_vals;

typedef struct {
  uint32_t n_buckets;
  uint32_t n_elems;
  const uint32_t keysize;
  const uint32_t valsize;
  const eq_cmp compare_newkey;
  const hasher hash_newkey;
  const hasher hash_storedkey;
  ahm_keys_and_vals *data;
} a_hashmap;

#define ahm_new(keytype, valtype, ...)                                         \
  __ahm_new(sizeof(keytype), sizeof(valtype), __VA_ARGS__)

a_hashmap __ahm_new(uint32_t keysize, uint32_t valsize, eq_cmp cmp_newkey,
                    hasher hash_newkey, hasher hash_storedkey);

uint32_t rotate_left(uint32_t n, uint32_t d);
uint32_t hash_eight_bytes(uint32_t seed, uint64_t bytes);
uint32_t hash_bytes(uint32_t seed, uint8_t *bytes, uint32_t n_bytes);

void *ahm_lookup(a_hashmap *hm, const void *key, void *context);
bool ahm_upsert(a_hashmap *hm, const void *key, const void *key_stored,
                const void *val, void *context);

// Don't use a proto-key, use a real key
void ahm_insert_stored(a_hashmap *hm, const void *key_stored, const void *val,
                       void *context);
void ahm_insert(a_hashmap *hm, const void *key, const void *key_stored,
                const void *val, void *context);
void ahm_free(a_hashmap *hm);
