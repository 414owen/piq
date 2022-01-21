#pragma once

#include <stdint.h>
#include <assert.h>

#include "util.h"

#define VEC_DECL(type) typedef struct { uint32_t len; uint32_t cap; type *data; } vec_ ## type;

#define VEC_NEW { .len = 0, .cap = 0, .data = NULL }

#define VEC_RESIZE(vec, _cap) \
  (vec)->data = realloc((vec)->data, (_cap) * sizeof((vec)->data[0])); \
  assert((vec)->data != NULL); \
  (vec)->cap = (_cap);

#define VEC_GROW(vec, _cap) if ((vec)->cap < (_cap)) { VEC_RESIZE((vec), (_cap)); }

#define VEC_PUSH(vec, el) \
  if ((vec)->cap == (vec)->len) { VEC_RESIZE((vec), MAX(20, (vec)->len * 2)); } \
  (vec)->data[(vec)->len++] = (el);

#define VEC_POP(vec) (vec)->len--;
  
#define VEC_BACK(vec) ((vec)->data[(vec)->len - 1])

#define VEC_FREE(vec) { if ((vec)->data != NULL) free((vec)->data); }

#define VEC_CLONE(vec) ((typeof(*vec)) { \
    .len = (vec)->len, \
    .cap = (vec)->cap, \
    .data = (vec)->cap == 0 ? NULL : memclone((vec)->data, sizeof((vec)->data[0]) * (vec)->cap) \
  })

typedef char* string;
VEC_DECL(string);