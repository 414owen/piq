#pragma once

#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "util.h"

#define VEC_FIRST_SIZE 10

#define VEC_DECL(type) typedef struct { uint32_t len; uint32_t cap; type *data; } vec_ ## type;

VEC_DECL(void);

#define VEC_NEW { .len = 0, .cap = 0, .data = NULL }

void __vec_resize(vec_void *vec, size_t cap, size_t elemsize);

#define VEC_RESIZE(vec, _cap) __vec_resize((vec_void*) vec, _cap, sizeof((vec)->data[0]))

void __vec_grow(vec_void *vec, size_t cap, size_t elemsize);

#define VEC_GROW(vec, _cap) __vec_grow((vec_void*) vec, _cap, sizeof((vec)->data[0]))

void __vec_push(vec_void *vec, void *el, size_t elemsize);

#define VEC_PUSH(vec, el) { \
  assert(sizeof((vec)->data[0]) == sizeof(el)); \
  typeof(el) __el = el; \
  __vec_push((vec_void*) vec, (void*) &__el, sizeof(el)); \
}

#define VEC_POP_(vec) --(vec)->len

#define VEC_POP(vec) ((vec)->data[--(vec)->len])
  
#define VEC_BACK(vec) ((vec)->data[(vec)->len - 1])

#define VEC_FREE(vec) { if ((vec)->data != NULL) { free((vec)->data); (vec)->data = NULL; } }

#define VEC_CLONE(vec) ((typeof(*vec)) { \
    .len = (vec)->len, \
    .cap = (vec)->cap, \
    .data = (vec)->cap == 0 ? NULL : memclone((vec)->data, sizeof((vec)->data[0]) * (vec)->cap) \
  })

#define VEC_FINALIZE(vec) { \
    (vec)->cap = (vec)->len; \
    (vec)->data = realloc((vec)->data, (vec)->len * sizeof((vec)->data[0])); \
  }

void __vec_append(vec_void *vec, void *els, size_t amt, size_t elemsize);

#define VEC_APPEND(vec, amt, els) { \
    assert(sizeof((vec)->data[0]) == sizeof(els[0])); \
    __vec_append((vec_void*) vec, (void*) els, amt, sizeof(els[0])); \
  }

void __vec_replicate(vec_void *vec, void *el, size_t amt, size_t elemsize);

// appends el to vec amt times
#define VEC_REPLICATE(vec, amt, el) { \
    assert(sizeof((vec)->data[0]) == sizeof(el)); \
    typeof(el) __el = el; \
    __vec_replicate((vec_void*) vec, (void*) &__el, amt, sizeof(el)); \
  }

#define VEC_CAT(v1, v2) VEC_APPEND((v1), (v2)->len, (v2)->data)

typedef char* string;

VEC_DECL(string);