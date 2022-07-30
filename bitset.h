#pragma once

#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>

typedef char* bitset_data;

typedef struct {
  bitset_data data;
  // in bits
  size_t len;
  // in bytes
  size_t cap;
} bitset;

#define BITSET_REQUIRED_BYTES(bits) ((bits) / CHAR_BIT + ((bits) % 8 > 0 ? 1 : 0))

void bs_free(bitset *bs);
bitset bs_new(void);
void bs_resize(bitset *bs, size_t size);
void bs_grow(bitset *bs, size_t size);
bool bs_data_get(bitset_data data, size_t ind);
bool bs_get(bitset bs, size_t ind);
void bs_data_set(bitset_data bs, size_t ind, bool b);
void bs_set(bitset bs, size_t ind, bool b);
void bs_push(bitset *bs, bool bit);
void bs_pop(bitset *bs);
