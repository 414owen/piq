#pragma once

#include <xxhash.h>

#include "consts.h"

typedef enum {
  SOURCE_FILE,
  SOURCE_STDIN,
  SOURCE_HARDCODED_TEST,
} source_type;

typedef struct {
  buf_ind_t length;
  XXH64_hash_t hash;
} source_digest;

typedef struct {
  source_type type;
  const char *restrict path;
  const char *restrict data;
  source_digest digest;
} source_file;

source_digest digest_from_string(const char *str);
