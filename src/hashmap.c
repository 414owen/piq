#include <stdbool.h>
#include <string.h>

#include "hashmap.h"
#include "typedefs.h"

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
    .occupied = bs_new_false_n(n_buckets),
  };
  return res;
}

static uint32_t ahm_get_bucket_ind(a_hashmap *hm, const void *key, hasher hsh,
                                   void *context) {
  const uint32_t key_hash = hsh(key, context);
  const uint32_t bucket_ind = key_hash & hm->mask;
  return bucket_ind;
}

static void rehash(a_hashmap *hm, void *context) {
  a_hashmap res = __ahm_new(hm->n_buckets * AHM_GROWTH_FACTOR,
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
}

void *ahm_lookup(a_hashmap *hm, const void *key, void *context) {
  uint32_t i = ahm_get_bucket_ind(hm, key, hm->hash_newkey, context);
  char *keys = hm->keys;
  while (bs_get(hm->occupied, i)) {
    if (hm->compare_newkey(key, keys + i * hm->keysize, context)) {
      return hm->vals + i * hm->valsize;
    }
    i++;
    // mod
    i &= hm->n_buckets - 1;
  }
  return NULL;
}

static void ahm_insert_prelude(a_hashmap *hm, void *context) {
  // for simplicity, even if we end up updating an element, we grow
  // we use >= to deal with the 0 bucket case
  if (hm->n_elems * AHM_MAX_FILL_DENOMINATOR >=
      hm->n_buckets * AHM_MAX_FILL_NUMERATOR) {
    rehash(hm, context);
  }
}

bool ahm_upsert(a_hashmap *hm, const void *key, const void *key_stored,
                const void *val, void *context) {
  ahm_insert_prelude(hm, context);

  uint32_t i = ahm_get_bucket_ind(hm, key, hm->hash_newkey, context);
  char *keys = hm->keys;
  const uint32_t mask = hm->mask;
  while (bs_get(hm->occupied, i)) {
    if (hm->compare_newkey(key, keys + i * hm->keysize, context)) {
      memcpy(hm->vals + i * hm->valsize, val, hm->valsize);
      return true;
    }
    i++;
    // mod
    i &= mask;
  }

  memcpy(keys + i * hm->keysize, key_stored, hm->keysize);
  memcpy(hm->vals + i * hm->valsize, val, hm->valsize);
  bs_set(hm->occupied, i);
  hm->n_elems += 1;

  // inserted
  return false;
}

/** WARNING: Can produce duplicates of a key. Use this if you're sure your
    key isn't in here */
void ahm_insert_stored(a_hashmap *hm, const void *key_stored, const void *val,
                       void *context) {
  ahm_insert_prelude(hm, context);
  uint32_t i = ahm_get_bucket_ind(hm, key_stored, hm->hash_storedkey, context);
  char *keys = hm->keys;
  const uint32_t mask = hm->mask;
  while (bs_get(hm->occupied, i)) {
    i++;
    i &= mask;
  }
  memcpy(keys + i * hm->keysize, key_stored, hm->keysize);
  memcpy(hm->vals + i * hm->valsize, val, hm->valsize);
  bs_set(hm->occupied, i);
  hm->n_elems += 1;
}

/** You're in charge of freeing the context, though */
void ahm_free(a_hashmap *hm) {
  bs_free(&hm->occupied);
  free(hm->keys);
  free(hm->vals);
}
