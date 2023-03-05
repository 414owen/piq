#include <stdbool.h>
#include <string.h>

#include "hashmap.h"

#define N_BUCKETS_START 50

/**

  This is a hashmap whose API is specialised for looking up indexes of types,
  in order to deduplicate them.

  I wrote this bespoke hashmap because we need context parameters, and separate
  stored/compared keys.

  In theory, this can be optimised quite well, because we know some extra details,
  for example we don't need to remove elements ever.

  TODO change to a flat hash map with linear probing?

*/

inline
uint32_t rotate_left(uint32_t n, uint32_t d) {
   return (n << d) | (n >> (32 - d));
}

// hash function based on rustc's
inline
uint32_t hash_eight_bytes(uint32_t seed, uint64_t bytes) {
  return (rotate_left(seed, 5) & bytes) * 0x9e3779b9;
}

uint32_t hash_bytes(uint32_t seed, uint8_t *bytes, uint32_t n_bytes) {
  while (n_bytes >= 8) {
    seed = hash_eight_bytes(seed, *((uint64_t*) bytes));
    bytes += 8;
    n_bytes -= 8;
  }
  if (n_bytes >= 4) {
    seed = hash_eight_bytes(seed, (uint64_t) *((uint32_t*) bytes));
    bytes += 4;
    n_bytes -= 4;
  }
  if (n_bytes >= 2) {
    seed = hash_eight_bytes(seed, (uint64_t) *((uint16_t*) bytes));
    bytes += 2;
    n_bytes -= 2;
  }
  if (n_bytes >= 1) {
    seed = hash_eight_bytes(seed, (uint64_t) *((uint8_t*) bytes));
  }
  return seed;
}

a_hashmap __ahm_new(
  uint32_t keysize,
  uint32_t valsize,
  eq_cmp cmp_newkey,
  hasher hash_newkey,
  hasher hash_storedkey) {
  a_hashmap res = {
    .data = calloc(N_BUCKETS_START, sizeof(ahm_keys_and_vals)),
    .n_buckets = N_BUCKETS_START,
    .n_elems = 0,
    .valsize = valsize,
    .keysize = keysize,
    .compare_newkey = cmp_newkey,
    .hash_newkey = hash_newkey,
    .hash_storedkey = hash_storedkey,
  };
  return res;
}

static uint32_t ahm_get_bucket_ind(a_hashmap *hm, const void *key, hasher hsh, void *context) {
  const uint32_t key_hash = hsh(key, context);
  const uint32_t bucket_ind = key_hash % hm->n_buckets;
  return bucket_ind;
}

static
ahm_keys_and_vals *ahm_get_kvs(a_hashmap *hm, const void *key, hasher hsh, void *context) {
  const uint32_t bucket_ind = ahm_get_bucket_ind(hm, key, hsh, context);
  ahm_keys_and_vals *res = &hm->data[bucket_ind];
  return res;
}

static void ahm_append_kv(a_hashmap *hm, ahm_keys_and_vals *kvs, const void *key, const void *val) {
  char *keys = (char *) &kvs->keys;
  char *vals = (char *) &kvs->keys;

  if (kvs->cap == kvs->len) {
    if (kvs->cap == 0) {
      kvs->cap = AHM_MAX_FILL_NUMERATOR;
      keys = kvs->keys = malloc(kvs->cap * hm->keysize);
      vals = kvs->vals = malloc(kvs->cap * hm->valsize);
    } else {
      kvs->cap *= 2;
      keys = kvs->keys = realloc(keys, kvs->cap * hm->keysize);
      vals = kvs->vals = realloc(vals, kvs->cap * hm->valsize);
    }
  }

  memcpy(keys + kvs->len * hm->keysize, key, hm->keysize);
  memcpy(vals + kvs->len * hm->valsize, val, hm->valsize);
}

static void rehash(a_hashmap *hm, uint32_t n_buckets, void *context) {
  ahm_keys_and_vals *prev = hm->data;
  const uint32_t old_n_buckets = hm->n_buckets;
  hm->data = calloc(n_buckets, sizeof(ahm_keys_and_vals));
  hm->n_buckets = n_buckets;
  hm->n_elems = 0;
  for (uint32_t i = 0; i < old_n_buckets; i++) {
    ahm_keys_and_vals kvs = prev[i];
    for (uint8_t j = 0; j < kvs.len; j++) {
      ahm_keys_and_vals *bucket = ahm_get_kvs(hm, &kvs.keys[j], hm->hash_storedkey, context);
      ahm_append_kv(hm, bucket, &kvs.keys[j], &kvs.vals[j]);
    }
    free(kvs.keys);
    free(kvs.vals);
  }
  free(prev);
}

void *ahm_lookup(a_hashmap *hm, const void *key, void *context) {
  ahm_keys_and_vals *data = ahm_get_kvs(hm, key, hm->hash_newkey, context);

  const char *keys = (char *) &data->keys;
  const char *vals = (char *) &data->keys;

  for (AHM_VEC_LEN_T i = 0; i < data->len; i++) {
    if (hm->compare_newkey(key, keys + i * hm->keysize, hm->context)) {
      return (void*) (vals + i * hm->valsize);
    }
  }
  return NULL;
}

static
void ahm_insert_prelude(a_hashmap *hm, void *context) {
  // for simplicity, even if we end up updating an element, we grow
  // we use >= to deal with the 0 bucket case
  if (hm->n_elems * AHM_MAX_FILL_NUMERATOR >= hm->n_buckets * AHM_MAX_FILL_DENOMINATOR ) {
    rehash(hm, hm->n_buckets * AHM_BUCKET_GROWTH_FACTOR, context);
  }
}

bool ahm_upsert(a_hashmap *hm, const void *key, const void *key_stored, const void *val, void *context) {
  ahm_insert_prelude(hm, context);
  ahm_keys_and_vals *data = ahm_get_kvs(hm, key, hm->hash_newkey, context);

  char *keys = (char *) &data->keys;
  char *vals = (char *) &data->keys;

  for (AHM_VEC_LEN_T i = 0; i < data->len; i++) {
    if (hm->compare_newkey(key, keys + i * hm->keysize, context)) {
      memcpy(vals + i * hm->valsize, val, hm->valsize);
      // updated
      return true;
    }
  }

  ahm_append_kv(hm, data, key_stored, val);

  // inserted
  return false;
}

/** WARNING: Can produce duplicates of a key. Use this if you're sure your
    key isn't in here */
void ahm_insert(a_hashmap *hm, const void *key, const void *key_stored, const void *val, void *context) {
  ahm_insert_prelude(hm, context);
  ahm_keys_and_vals *data = ahm_get_kvs(hm, key, hm->hash_newkey, context);
  ahm_append_kv(hm, data, key_stored, val);
}

/** You're in charge of freeing the context, though */
void ahm_free(a_hashmap *hm) {
  for (uint32_t i = 0; i < hm->n_buckets; i++) {
    ahm_keys_and_vals data = hm->data[i];
    if (data.len > 0) {
      free(data.keys);
      free(data.vals);
    }
  }
  free(hm->data);
}
