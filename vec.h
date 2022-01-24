#pragma once

#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "util.h"

#define VEC_DECL(type) typedef struct { uint32_t len; uint32_t cap; type *data; } vec_ ## type;

#define VEC_NEW { .len = 0, .cap = 0, .data = NULL }

#define VEC_RESIZE(vec, _cap) { \
    (vec)->data = realloc((vec)->data, (_cap) * sizeof((vec)->data[0])); \
    assert((vec)->data != NULL); \
    (vec)->cap = (_cap); \
  }

#define VEC_GROW(vec, _cap) if ((vec)->cap < (_cap)) { VEC_RESIZE((vec), (_cap)); }

#define VEC_PUSH(vec, el) { \
    if ((vec)->cap == (vec)->len) { VEC_RESIZE((vec), MAX(20, (vec)->len * 2)); } \
    (vec)->data[(vec)->len++] = (el); \
  }

#define VEC_POP_(vec) --(vec)->len

#define VEC_POP(vec) ((vec)->data[--(vec)->len])
  
#define VEC_BACK(vec) ((vec)->data[(vec)->len - 1])

#define VEC_FREE(vec) { if ((vec)->data != NULL) free((vec)->data); }

#define VEC_CLONE(vec) ((typeof(*vec)) { \
    .len = (vec)->len, \
    .cap = (vec)->cap, \
    .data = (vec)->cap == 0 ? NULL : memclone((vec)->data, sizeof((vec)->data[0]) * (vec)->cap) \
  })

#define VEC_FINALIZE(vec) { \
    (vec)->cap = (vec)->len; \
    (vec)->data = realloc((vec)->data, (vec)->len * sizeof((vec)->data[0])); \
  }

#define VEC_APPEND(v1, amt, els) { \
    VEC_GROW((v1), (amt) + (v1)->len); \
    memcpy(&(v1)->data[(v1)->len], (els), amt * sizeof((v1)->data[0])); \
    v1->len += amt; \
  }

#define VEC_CAT(v1, v2) VEC_APPEND((v1), (v2)->len, (v2)->data)

typedef char* string;

VEC_DECL(string);