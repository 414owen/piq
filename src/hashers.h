#pragma once

#include <stdint.h>

uint32_t hash_newtype(const void *key_p, const void *ctx_p);
uint32_t hash_stored_type(const void *key_p, const void *ctx_p);
uint32_t hash_binding(const void *binding_p, const void *ctx_p);
uint32_t hash_stored_binding(const void *binding_ind_p, const void *ctx_p);
