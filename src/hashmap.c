#include <hedley.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "consts.h"
#include "hashmap.h"
#include "typedefs.h"
#include "util.h"

// max ratio of elems to buckets before rehash
// eg. 4/5 is four elements for five buckets
#define AHM_MAX_FILL_NUMERATOR 3
#define AHM_MAX_FILL_DENOMINATOR 4

// Keep me a power of two please
#define AHM_GROWTH_FACTOR 2

/**

  This is a hashmap whose API is specialised for looking up indexes of types,
  in order to deduplicate them.

  I wrote this bespoke hashmap because we need context parameters, and separate
  stored/compared keys.

  In theory, this can be optimised quite well, because we know some extra
  details, for example we don't need to remove elements ever.

  TODO change to a flat hash map with linear probing?

*/

a_hashmap __ahm_new(uint32_t n_buckets, uint32_t keysize, uint32_t valsize,
                    eq_cmp cmp_newkey, hasher hash_newkey,
                    hasher hash_storedkey) {
  a_hashmap res = {
    .keys = calloc(n_buckets, keysize),
    .vals = calloc(n_buckets, valsize),
    .n_buckets = n_buckets,
    .mask = n_buckets - 1,
    .n_elems = 0,
    .valsize = valsize,
    .keysize = keysize,
    .compare_newkey = cmp_newkey,
    .hash_newkey = hash_newkey,
    .hash_storedkey = hash_storedkey,
    // TODO manage these ourselves to avoid duplicate `length`, `capacity` etc.
    .occupied = bs_new_false_n(n_buckets),
    // if this is true, occupied is false
    .tombstoned = bs_new_false_n(n_buckets),
  };
  return res;
}

static uint32_t ahm_get_bucket_ind(a_hashmap *hm, const void *key, hasher hsh,
                                   const void *context) {
  const uint32_t key_hash = hsh(key, context);
  const uint32_t bucket_ind = key_hash & hm->mask;
  return bucket_ind;
}

static void rehash(a_hashmap *hm, void *context) {
  // printf("\n\nrehash\n");
  u32 n_buckets = hm->n_buckets;
  u32 new_num_buckets = n_buckets * AHM_GROWTH_FACTOR;
  if (n_buckets > (UINT32_MAX >> 1)) {
    if (n_buckets == UINT32_MAX) {
      printf("Ran out of hashmap space.\n"
             "Please report this, at %s\n",
             issue_tracker_url);
      exit(1);
    }
    new_num_buckets = UINT32_MAX;
  }
  a_hashmap res = __ahm_new(new_num_buckets,
                            hm->keysize,
                            hm->valsize,
                            hm->compare_newkey,
                            hm->hash_newkey,
                            hm->hash_storedkey);
  for (u32 i = 0; i < hm->n_buckets; i++) {
    if (bs_get(hm->occupied, i)) {
      ahm_insert_stored(
        &res, hm->keys + i * hm->keysize, hm->vals + i * hm->valsize, context);
    }
  }
  ahm_free(hm);
  hm->n_buckets = res.n_buckets;
  hm->mask = res.mask;
  hm->keys = res.keys;
  hm->vals = res.vals;
  hm->occupied = res.occupied;
  hm->tombstoned = res.tombstoned;
}

static bool ahm_memcmp_keys(const void *key1, const void *key2,
                            const void *keysize_p) {
  uint32_t keysize = *((uint32_t *)keysize_p);
  return memcmp(key1, key2, keysize) == 0;
}

static u32 ahm_lookup_internal(a_hashmap *hm, const void *key, const hasher hsh,
                               const eq_cmp cmp, const void *hash_ctx,
                               const void *cmp_ctx) {
  const uint32_t mask = hm->mask;
  const char *keys = hm->keys;
  uint32_t i = ahm_get_bucket_ind(hm, key, hsh, hash_ctx);
  uint32_t first_empty = UINT32_MAX;
  do {
    while (bs_get(hm->occupied, i)) {
      if (cmp(key, keys + i * hm->keysize, cmp_ctx)) {
        return i;
      }
      i++;
      // mod
      i &= mask;
    }
    first_empty = MIN(first_empty, i);
    if (bs_get(hm->tombstoned, i)) {
      i++;
      i &= mask;
    } else {
      return first_empty;
    }
  } while (true);
}

static u32 ahm_lookup_stored(a_hashmap *hm, const void *key, void *context) {
  return ahm_lookup_internal(
    hm, key, hm->hash_storedkey, ahm_memcmp_keys, context, &hm->keysize);
}

u32 ahm_lookup(a_hashmap *hm, const void *key, void *context) {
  return ahm_lookup_internal(
    hm, key, hm->hash_newkey, hm->compare_newkey, context, context);
}

void ahm_maybe_rehash(a_hashmap *hm, void *context) {
  // for simplicity, even if we end up updating an element, we grow
  // we use >= to deal with the 0 bucket case

  // TODO store these values in hashmap struct
  if (hm->n_elems * AHM_MAX_FILL_DENOMINATOR >=
      hm->n_buckets * AHM_MAX_FILL_NUMERATOR) {
    rehash(hm, context);
  }
}

void ahm_upsert(a_hashmap *hm, const void *key, const void *key_stored,
                const void *val, void *context) {
  ahm_maybe_rehash(hm, context);
  const u32 bucket_ind = ahm_lookup_internal(
    hm, key, hm->hash_newkey, hm->compare_newkey, context, context);
  ahm_insert_at(hm, bucket_ind, key_stored, val);
}

/** WARNING: Can produce duplicates of a key. Use this if you're sure your
    key isn't in here */
void ahm_insert_stored(a_hashmap *hm, const void *key_stored, const void *val,
                       void *context) {
  ahm_maybe_rehash(hm, context);
  uint32_t i = ahm_get_bucket_ind(hm, key_stored, hm->hash_storedkey, context);
  const uint32_t mask = hm->mask;
  while (bs_get(hm->occupied, i)) {
    i++;
    i &= mask;
  }
  ahm_insert_at(hm, i, key_stored, val);
}

/** Warning, doesn't trigger rehash, so you'll want to call ahm_maybe_rehash
   unless you've actually removed something
*/
void ahm_insert_at(a_hashmap *hm, u32 index, const void *key_stored,
                   const void *val) {
  memcpy(hm->keys + index * hm->keysize, key_stored, hm->keysize);
  memcpy(hm->vals + index * hm->valsize, val, hm->valsize);
  bs_set(hm->occupied, index);
  bs_clear(hm->tombstoned, index);
  hm->n_elems++;
}

static uint32_t ahm_remove_epilogue(a_hashmap *hm, uint32_t ind) {
  if (HEDLEY_LIKELY(bs_get(hm->occupied, ind))) {
    bs_set(hm->tombstoned, ind);
    bs_clear(hm->occupied, ind);
    hm->n_elems--;
  }
  return ind;
}

u32 ahm_remove(a_hashmap *hm, const void *key, void *context) {
  const u32 ind = ahm_lookup(hm, key, context);
  return ahm_remove_epilogue(hm, ind);
}

u32 ahm_remove_stored(a_hashmap *hm, const void *key, void *context) {
  const u32 ind = ahm_lookup_stored(hm, key, context);
  return ahm_remove_epilogue(hm, ind);
}

/** You're in charge of freeing the context, though */
void ahm_free(a_hashmap *hm) {
  bs_free(&hm->occupied);
  bs_free(&hm->tombstoned);
  free(hm->keys);
  free(hm->vals);
}
