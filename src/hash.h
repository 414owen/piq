#pragma once

#include <xxhash.h>

typedef XXH64_hash_t hash_t;

hash_t hash(const char *data, size_t length);
