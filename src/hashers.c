// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

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
#define PRIME_1 (sizeof(hash_t) == 8 ? 0x9E3779B185EBCA87ULL : 4201133899)
/* 0b1100001010110010101011100011110100100111110101001110101101001111 */
#define PRIME_2 (sizeof(hash_t) == 8 ? 0xC2B2AE3D27D4EB4FULL : 3716661401)
/* 0b0001011001010110011001111011000110011110001101110111100111111001 */
#define PRIME_3 (sizeof(hash_t) == 8 ? 0x165667B19E3779F9ULL : 2561059601)
/* 0b1000010111101011110010100111011111000010101100101010111001100011 */
#define PRIME_4 (sizeof(hash_t) == 8 ? 0x85EBCA77C2B2AE63ULL : 3647556947)
/* 0b0010011111010100111010110010111100010110010101100110011111000101 */
#define PRIME_5 (sizeof(hash_t) == 8 ? 0x27D4EB2F165667C5ULL : 4164797977)

// This doesn't seem to make a big difference
#define INITIAL_SEED PRIME_2

// rotate right
static inline hash_t rorhash_t(hash_t v, int r) {
  return (v >> r) | (v << ((CHAR_BIT * sizeof(v)) - r));
}

HEDLEY_INLINE
static hash_t mix(hash_t v) {
  v ^= rorhash_t(v, sizeof(hash_t) == 8 ? 49 : 17) ^ rorhash_t(v, 24);
  v *= PRIME_1;
  v ^= v >> 28;
  v *= PRIME_3;
  return v ^ v >> 28;
}

HEDLEY_INLINE
static hash_t hash_hash_t_bytes(hash_t seed, hash_t data) {
  // Adding data to the seed just in case we lose data
  // from mixing. Doesn't seem to to much in practice.
  return (data + seed) ^ mix(data);
}

static hash_t hash_bytes(hash_t seed, const uint8_t *restrict bytes,
                         uint32_t n_bytes) {
  assert(sizeof(hash_t) <= 8);
  while (n_bytes >= sizeof(hash_t)) {
    seed = hash_hash_t_bytes(seed, *((hash_t *)bytes));
    bytes += sizeof(hash_t);
    n_bytes -= sizeof(hash_t);
  }
#if HASH_BITS == 64
  if (n_bytes >= 4) {
    seed = hash_hash_t_bytes(seed, (hash_t)(*((uint32_t *)bytes)));
    bytes += 4;
    n_bytes -= 4;
  }
#endif
  if (n_bytes >= 2) {
    seed = hash_hash_t_bytes(seed, (hash_t)(*((uint16_t *)bytes)));
    bytes += 2;
    n_bytes -= 2;
  }
  if (n_bytes == 1) {
    seed = hash_hash_t_bytes(seed, (hash_t)(*((uint8_t *)bytes)));
  }
  return seed;
}

static hash_t hash_string(hash_t seed, const char *str) {
  return hash_bytes(seed, (uint8_t *)str, strlen(str));
}

#define hash_primitive(seed, val)                                              \
  (sizeof(val) <= sizeof(hash_t)                                               \
     ? hash_hash_t_bytes(seed, (val))                                          \
     : hash_bytes(seed, (uint8_t *)(&(val)), sizeof(val)))

hash_t hash_newtype(const void *key_p, const void *ctx) {
  type_key_with_ctx *key = (type_key_with_ctx *)key_p;
  hash_t hash = hash = hash_primitive(INITIAL_SEED, key->tag);
  switch (type_repr(key->tag)) {
    case SUBS_EXTERNAL:
      hash =
        hash_bytes(hash, (uint8_t *)key->subs, key->sub_amt * sizeof(type_ref));
      break;
    case SUBS_NONE:
      break;
    case SUBS_ONE:
      hash = hash_primitive(hash, key->sub_a);
      break;
    case SUBS_TWO:
      hash = hash_primitive(hash, key->sub_a);
      hash = hash_primitive(hash, key->sub_b);
      break;
  }
  return hash;
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
  hash_t hash = hash_primitive(INITIAL_SEED, key.tag.check);
  switch (type_repr(key.tag.check)) {
    case SUBS_EXTERNAL: {
      type_ref *subs = &VEC_DATA_PTR(&builder->inds)[key.data.more_subs.start];
      hash = hash_bytes(
        hash, (uint8_t *)subs, key.data.more_subs.amt * sizeof(type_ref));
      break;
    }
    case SUBS_NONE:
      break;
    case SUBS_ONE:
      hash = hash_primitive(hash, key.data.one_sub.ind);
      break;
    case SUBS_TWO:
      hash = hash_primitive(hash, key.data.two_subs.a);
      hash = hash_primitive(hash, key.data.two_subs.b);
      break;
  }
  return hash;
}
