#pragma once

#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>

typedef char *bitset_data;

typedef struct {
  bitset_data data;
  // in bits
  size_t len;
  // in bytes
  size_t cap;
} bitset;

#define BITMASK(b) (1 << ((b) % CHAR_BIT))
#define BITSLOT(b) ((b) / CHAR_BIT)
#define BITSET(a, b) ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITCLEAR(a, b) ((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a, b) ((a)[BITSLOT(b)] & BITMASK(b))
#define BITNSLOTS(nb) ((nb + CHAR_BIT - 1) / CHAR_BIT)

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
