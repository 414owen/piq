#pragma once

#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>

// bits
#define BITSET_INITIAL_SIZE 64
// 1.5x
#define BITSET_APPLY_GROWTH_FACTOR(cap) ((cap) + ((cap) >> 1))

typedef char *restrict bitset_data;

typedef struct {
  // TODO store stuff inline here and with cap
  bitset_data data;
  // in bits
  size_t len;
  // in bytes
  size_t cap;
} bitset;

// gcc turns this into an & 7
#define BITMASK(b) (1 << ((b) % CHAR_BIT))
#define BITSLOT(b) ((b) / CHAR_BIT)
#define BITSET(a, b) ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITCLEAR(a, b) ((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a, b) ((a)[BITSLOT(b)] & BITMASK(b))
#define BITNSLOTS(nb) ((nb + CHAR_BIT - 1) / CHAR_BIT)

void bs_free(bitset *bs);
bitset bs_new(void);
bitset bs_new_false_n(size_t n);
void bs_resize(bitset *bs, size_t size);
void bs_grow(bitset *bs, size_t size);
// bool bs_data_get(bitset_data data, size_t ind);
bool bs_get(bitset bs, size_t ind);
void bs_data_set(bitset_data bs, size_t ind);
void bs_data_update(bitset_data bs, size_t ind, bool b);
void bs_set(bitset bs, size_t ind);
void bs_clear(bitset bs, size_t ind);
void bs_update(bitset bs, size_t ind, bool b);
void bs_push_true(bitset *bs);
void bs_push_true_n(bitset *bs, size_t amt);
void bs_push_false(bitset *bs);
void bs_push_false_n(bitset *bs, size_t amt);
void bs_push(bitset *bs, bool bit);
bool bs_pop(bitset *bs);
bool bs_peek(bitset *bs);
void bs_pop_n(bitset *bs, size_t n);
