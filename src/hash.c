#include "hash.h"

// just to separate the implementation from the API
hash_t hash(const char *data, size_t length) {
  return XXH3_64bits(data, length);
}
