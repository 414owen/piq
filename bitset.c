#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

#include "bitset.h"
#include "vec.h"

#define BITMASK(b) (1 << ((b) % CHAR_BIT))
#define BITSLOT(b) ((b) / CHAR_BIT)
#define BITSET(a, b) ((a)[BITSLOT(b)] |= BITMASK(b))
#define BITCLEAR(a, b) ((a)[BITSLOT(b)] &= ~BITMASK(b))
#define BITTEST(a, b) ((a)[BITSLOT(b)] & BITMASK(b))
#define BITNSLOTS(nb) ((nb + CHAR_BIT - 1) / CHAR_BIT)

bitset bs_new(void) {
  bitset res = {
    .len = 0,
    .cap = 0,
    .data = NULL,
  };
  return res;
}

void bs_resize(bitset *bs, size_t bits) {
  size_t bytes_needed = bits / CHAR_BIT + (bits % 8 > 0 ? 1 : 0);
  bs->data = realloc(bs->data, bytes_needed);
  debug_assert(bs->data != NULL);
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
  return (data[ind / 8] & (1 << (ind % 8))) > 0;
}

bool bs_get(bitset bs, size_t ind) { return bs_data_get(bs.data, ind); }

void bs_data_set(bitset_data data, size_t ind, bool b) {
  if (b) {
    data[BITSLOT(ind)] |= BITMASK(ind);
  } else {
    data[BITSLOT(ind)] &= ~(BITMASK(ind));
  }
}

void bs_set(bitset bs, size_t ind, bool b) { bs_data_set(bs.data, ind, b); }

void bs_push(bitset *bs, bool bit) {
  bs_grow(bs, bs->len + 1);
  bs_data_set(bs->data, bs->len++, bit);
}

void bs_free(bitset *bs) {
  free(bs->data);
  bs->data = NULL;
}
