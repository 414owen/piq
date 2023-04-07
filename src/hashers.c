#include <predef/predef.h>
#include <stdint.h>
#include <time.h>

#include "hashers.h"
#include "resolve_scope.h"
#include "types.h"

/*
 * This module defines all our hash functions in one place,
 * so that we get perf gains of it being in one compilation
 * unit.
 */

// TODO use AES / crypto instructions if present. I has a
// quick go at this, but it wasn't fun. Different processors
// have different instructions to do this sort of thing.
// I could write a library which abstracts hardware-compatible
// hashing, a la rust's aHash.

// Some useful numbers to play around with, borrowed from XXHASH
/* 0b1001111000110111011110011011000110000101111010111100101010000111 */
#define PRIME64_1 0x9E3779B185EBCA87ULL
/* 0b1100001010110010101011100011110100100111110101001110101101001111 */
#define PRIME64_2 0xC2B2AE3D27D4EB4FULL
/* 0b0001011001010110011001111011000110011110001101110111100111111001 */
#define PRIME64_3 0x165667B19E3779F9ULL
/* 0b1000010111101011110010100111011111000010101100101010111001100011 */
#define PRIME64_4 0x85EBCA77C2B2AE63ULL
/* 0b0010011111010100111010110010111100010110010101100110011111000101 */
#define PRIME64_5 0x27D4EB2F165667C5ULL

// This doesn't seem to make a big difference
#define INITIAL_SEED PRIME64_2

static inline uint64_t ror64(uint64_t v, int r) {
  return (v >> r) | (v << (64 - r));
}

HEDLEY_INLINE
static uint64_t mix(uint64_t v) {
  v ^= ror64(v, 49) ^ ror64(v, 24);
  v *= PRIME64_1;
  v ^= v >> 28;
  v *= PRIME64_3;
  return v ^ v >> 28;
}

HEDLEY_INLINE
static hash_t hash_eight_bytes(hash_t seed, u64 data) {
  // Adding data to the seed just in case we lose data
  // from mixing. Doesn't seem to to much in practice.
  return (data + seed) ^ mix(data);
}

static hash_t hash_bytes(hash_t seed, const uint8_t *restrict bytes,
                         uint32_t n_bytes) {
  while (n_bytes >= 8) {
    seed = hash_eight_bytes(seed, *((uint64_t *)bytes));
    bytes += 8;
    n_bytes -= 8;
  }
  if (n_bytes >= 4) {
    seed = hash_eight_bytes(seed, (uint64_t)(*((uint32_t *)bytes)));
    bytes += 4;
    n_bytes -= 4;
  }
  if (n_bytes >= 2) {
    seed = hash_eight_bytes(seed, (uint64_t)(*((uint16_t *)bytes)));
    bytes += 2;
    n_bytes -= 2;
  }
  if (n_bytes == 1) {
    seed = hash_eight_bytes(seed, (uint64_t)(*((uint8_t *)bytes)));
  }
  return seed;
}

static hash_t hash_string(hash_t seed, const char *str) {
  return hash_bytes(seed, (uint8_t *)str, strlen(str));
}

#define hash_primitive(seed, val)                                              \
  (sizeof(val) <= sizeof(hash_t)                                               \
     ? hash_eight_bytes(seed, (val))                                           \
     : hash_bytes(seed, (uint8_t *)(&(val)), sizeof(val)))

hash_t hash_newtype(const void *key_p, const void *ctx) {
  type_key_with_ctx *key = (type_key_with_ctx *)key_p;
  hash_t hash = INITIAL_SEED;
  switch (type_repr(key->tag)) {
    case SUBS_EXTERNAL: {
      hash = hash_primitive(hash, key->tag);
      return hash_bytes(
        hash, (uint8_t *)key->subs, key->sub_amt * sizeof(type_ref));
    }
    default: {
      hash = hash_primitive(hash, key->tag);
      hash = hash_primitive(hash, key->sub_a);
      return hash_primitive(hash, key->sub_b);
    }
  }
}

hash_t hash_binding(const void *binding_p, const void *ctx_p) {
  const binding bnd = *((binding *)binding_p);
  const char *source_file = ((resolve_map_ctx *)ctx_p)->source_file;
  return hash_bytes(INITIAL_SEED, (u8 *)source_file + bnd.start, bnd.len);
}

hash_t hash_stored_binding(const void *binding_ind_p, const void *ctx_p) {
  const node_ind_t bnd_ind = *((node_ind_t *)binding_ind_p);
  const resolve_map_ctx ctx = *((resolve_map_ctx *)ctx_p);
  str_ref sref = VEC_GET(ctx.scope->bindings, bnd_ind);
  return bs_get(ctx.scope->is_builtin, bnd_ind)
           ? hash_string(INITIAL_SEED, sref.builtin)
           : hash_bytes(INITIAL_SEED,
                        (u8 *)ctx.source_file + sref.binding.start,
                        sref.binding.len);
}

hash_t hash_stored_type(const void *key_p, const void *ctx_p) {
  type_ref key_ind = *((type_ref *)key_p);
  type_builder *builder = (type_builder *)ctx_p;
  type key = VEC_GET(builder->types, key_ind);
  hash_t hash = INITIAL_SEED;
  switch (type_repr(key.check_tag)) {
    case SUBS_EXTERNAL: {
      hash = hash_primitive(hash, key.tag);
      type_ref *subs = &VEC_DATA_PTR(&builder->inds)[key.subs_start];
      return hash_bytes(hash, (uint8_t *)subs, key.sub_amt * sizeof(type_ref));
    }
    default: {
      hash = hash_primitive(hash, key.tag);
      hash = hash_primitive(hash, key.sub_a);
      return hash_primitive(hash, key.sub_b);
    }
  }
}
