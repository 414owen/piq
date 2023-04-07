#pragma once

#include <stdint.h>

typedef uint64_t hash_t;

hash_t hash_newtype(const void *key_p, const void *ctx_p);
hash_t hash_stored_type(const void *key_p, const void *ctx_p);
hash_t hash_binding(const void *binding_p, const void *ctx_p);
hash_t hash_stored_binding(const void *binding_ind_p, const void *ctx_p);
