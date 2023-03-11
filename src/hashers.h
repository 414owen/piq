#pragma once

#include <stdint.h>

uint32_t hash_newtype(const void *key_p, const void *ctx);
uint32_t hash_stored_type(const void *key_p, const void *ctx_p);
