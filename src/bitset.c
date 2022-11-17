#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

#include "bitset.h"
#include "vec.h"

// TODO: Small bitsets should be stored inline

bitset bs_new(void) {
  bitset res = {
    .len = 0,
    .cap = 0,
    .data = NULL,
  };
  return res;
}

void bs_resize(bitset *bs, size_t bits) {
  size_t bytes_needed = BITNSLOTS(bits);
  bs->data = realloc(bs->data, bytes_needed);
  size_t cap = bs->cap;
  memset(&bs->data[cap], 0, bytes_needed - cap);
  bs->cap = bytes_needed;
}

void bs_grow(bitset *bs, size_t bits) {
  if (bits > CHAR_BIT * bs->cap) {
    size_t new_size = CHAR_BIT * MAX(VEC_FIRST_SIZE, bs->cap * 2);
    new_size = MAX(bits, new_size);
    bs_resize(bs, new_size);
  }
}

bool bs_data_get(bitset_data data, size_t ind) {
  debug_assert(data != NULL);
  return BITTEST(data, ind) > 0;
}

bool bs_get(bitset bs, size_t ind) { return bs_data_get(bs.data, ind); }

void bs_data_set(bitset_data data, size_t ind, bool b) {
  if (b) {
    BITSET(data, ind);
  } else {
    BITCLEAR(data, ind);
  }
}

void bs_set(bitset bs, size_t ind, bool b) { bs_data_set(bs.data, ind, b); }

void bs_push(bitset *bs, bool bit) {
  bs_grow(bs, bs->len + 1);
  bs_data_set(bs->data, bs->len++, bit);
}

bool bs_pop(bitset *bs) {
  size_t len = --bs->len;
  return bs_data_get(bs->data, len);
}

bool bs_peek(bitset *bs) { return bs_data_get(bs->data, bs->len - 1); }

void bs_pop_n(bitset *bs, size_t n) { bs->len -= n; }

void bs_free(bitset *bs) {
  free(bs->data);
  bs->data = NULL;
}
