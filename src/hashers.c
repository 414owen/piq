#include <stdint.h>

#include "types.h"

HEDLEY_INLINE
static uint32_t rotate_left(uint32_t n, uint32_t d) {
  return (n << d) | (n >> (32 - d));
}

// hash function based on rustc's
HEDLEY_INLINE
static uint32_t hash_eight_bytes(uint32_t seed, uint64_t bytes) {
  return (rotate_left(seed, 5) ^ bytes) * 0x9e3779b9;
}

static uint32_t hash_bytes(uint32_t seed, uint8_t *bytes, uint32_t n_bytes) {
  while (n_bytes >= 8) {
    seed = hash_eight_bytes(seed, *((uint64_t *)bytes));
    bytes += 8;
    n_bytes -= 8;
  }
  if (n_bytes >= 4) {
    seed = hash_eight_bytes(seed, (uint64_t) * ((uint32_t *)bytes));
    bytes += 4;
    n_bytes -= 4;
  }
  if (n_bytes >= 2) {
    seed = hash_eight_bytes(seed, (uint64_t) * ((uint16_t *)bytes));
    bytes += 2;
    n_bytes -= 2;
  }
  if (n_bytes == 1) {
    seed = hash_eight_bytes(seed, (uint64_t) * ((uint8_t *)bytes));
  }
  return seed;
}

uint32_t hash_newtype(const void *key_p, const void *ctx) {
  type_key_with_ctx *key = (type_key_with_ctx *)key_p;
  switch (type_repr(key->tag)) {
    case SUBS_EXTERNAL: {
      uint32_t hash = hash_eight_bytes(0, key->tag);
      return hash_bytes(
        hash, (uint8_t *)key->subs, key->sub_amt * sizeof(type_ref));
    }
    default: {
      HASH_TYPE h1 = hash_eight_bytes(0, key->tag);
      HASH_TYPE h2 = hash_eight_bytes(h1, key->sub_a);
      return hash_eight_bytes(h2, key->sub_b);
    }
  }
}

uint32_t hash_stored_type(const void *key_p, const void *ctx_p) {
  type_ref key_ind = *((type_ref *)key_p);
  type_builder *builder = (type_builder *)ctx_p;
  type key = VEC_GET(builder->types, key_ind);
  switch (type_repr(key.check_tag)) {
    case SUBS_EXTERNAL: {
      uint32_t hash = hash_eight_bytes(0, key.tag);
      type_ref *subs = &VEC_DATA_PTR(&builder->inds)[key.subs_start];
      return hash_bytes(hash, (uint8_t *)subs, key.sub_amt * sizeof(type_ref));
    }
    default: {
      HASH_TYPE h1 = hash_eight_bytes(0, key.tag);
      HASH_TYPE h2 = hash_eight_bytes(h1, key.sub_a);
      return hash_eight_bytes(h2, key.sub_b);
    }
  }
}
