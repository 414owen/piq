// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

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

void bs_resize(bitset *restrict bs, size_t bits) {
  size_t bytes_needed = BITNSLOTS(bits);
  bs->data = realloc(bs->data, bytes_needed);
  size_t cap = bs->cap;
  memset(&bs->data[cap], 0, bytes_needed - cap);
  bs->cap = bytes_needed;
}

void bs_grow(bitset *restrict bs, size_t bits) {
  if (bits > CHAR_BIT * bs->cap) {
    size_t new_size =
      CHAR_BIT * MAX(BITSET_INITIAL_SIZE, BITSET_APPLY_GROWTH_FACTOR(bs->cap));
    new_size = MAX(bits, new_size);
    bs_resize(bs, new_size);
  }
}

static bool bs_data_get(bitset_data data, size_t ind) {
  debug_assert(data != NULL);
  return BITTEST(data, ind) > 0;
}

bool bs_get(bitset bs, size_t ind) { return bs_data_get(bs.data, ind); }

void bs_data_set(bitset_data data, size_t ind) { BITSET(data, ind); }

void bs_data_clear(bitset_data data, size_t ind) { BITCLEAR(data, ind); }

void bs_data_update(bitset_data data, size_t ind, bool b) {
  if (b) {
    bs_data_set(data, ind);
  } else {
    bs_data_clear(data, ind);
  }
}

void bs_set(bitset bs, size_t ind) { bs_data_set(bs.data, ind); }

bool bs_get_set(bitset bs, size_t ind) {
  char *slot = &bs.data[BITSLOT(ind)];
  u8 mask = BITMASK(ind);
  bool res = (*slot) & mask;
  *slot |= mask;
  return res;
}

void bs_clear(bitset bs, size_t ind) { bs_data_clear(bs.data, ind); }

bool bs_get_clear(bitset bs, size_t ind) {
  char *slot = &bs.data[BITSLOT(ind)];
  u8 mask = BITMASK(ind);
  bool res = (*slot) & mask;
  *slot &= ~mask;
  return res;
}

void bs_update(bitset bs, size_t ind, bool b) {
  bs_data_update(bs.data, ind, b);
}

void bs_push_true(bitset *bs) {
  bs_grow(bs, bs->len + 1);
  bs_data_set(bs->data, bs->len);
  bs->len++;
}

void bs_push_false(bitset *bs) {
  bs_grow(bs, bs->len + 1);
  bs_data_clear(bs->data, bs->len);
  bs->len++;
}

void bs_push(bitset *bs, bool bit) {
  bs_grow(bs, bs->len + 1);
  bs_data_update(bs->data, bs->len, bit);
  bs->len++;
}

void bs_push_true_n(bitset *bs, size_t amt) {
  bs_grow(bs, bs->len + amt);
  // TODO remove loop
  for (size_t i = 0; i < 8 - (bs->len % 8); i++) {
    bs_data_set(bs->data, bs->len + i);
  }
  memset(bs->data + BITNSLOTS(bs->len),
         0xff,
         BITNSLOTS(bs->len + amt) - BITNSLOTS(bs->len));
  bs->len += amt;
}

void bs_push_false_n(bitset *bs, size_t amt) {
  bs_grow(bs, bs->len + amt);
  // TODO remove loop
  for (size_t i = 0; i < 8 - (bs->len % 8); i++) {
    bs_data_clear(bs->data, bs->len + i);
  }
  memset(bs->data + BITNSLOTS(bs->len),
         0x00,
         BITNSLOTS(bs->len + amt) - BITNSLOTS(bs->len));
  bs->len += amt;
}

bitset bs_new_false_n(size_t n) {
  size_t slots = BITNSLOTS(n);
  bitset res = {
    .data = calloc(slots, 1),
    .cap = slots,
    .len = n,
  };
  return res;
}

bool bs_pop(bitset *restrict bs) {
  debug_assert(bs->len > 0);
  size_t len = --bs->len;
  return bs_data_get(bs->data, len);
}

bool bs_peek(bitset *restrict bs) { return bs_data_get(bs->data, bs->len - 1); }

void bs_pop_n(bitset *restrict bs, size_t n) { bs->len -= n; }

void bs_free(bitset *restrict bs) {
  free(bs->data);
  bs->data = NULL;
}
