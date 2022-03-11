#pragma once

#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "util.h"

#define VEC_FIRST_SIZE 10

#ifndef INLINE_VEC_BYTES
#define INLINE_VEC_BYTES 204
#endif

#define SIZE_TO_INLINE_AMT(size) (INLINE_VEC_BYTES / size)
#define TYPE_TO_INLINE_AMT(type) (SIZE_TO_INLINE_AMT(sizeof(type)))
#define VEC_IS_INLINE(vec) ((vec)->len <= TYPE_TO_INLINE_AMT((vec)->data[0]))
#define VEC_IS_EXTERNAL(vec) ((vec)->len > TYPE_TO_INLINE_AMT((vec)->data[0]))
#define VEC_DATA_PTR(vec) (VEC_IS_EXTERNAL(vec) ? (vec)->data : (vec)->inline_data)
#define VEC_DATA_PTR(vec) (VEC_IS_EXTERNAL(vec) ? (vec)->data : (vec)->inline_data)

#define VEC_DECL(type) \
  typedef struct { \
    uint32_t len; \
    union { \
      type inline_data[TYPE_TO_INLINE_AMT(type)]; \
      struct { \
        uint32_t cap; \
        type *data; \
      }; \
    }; \
  } vec_ ## type

/*
 * len > inline_amt -> external
 * len <= inline_amt -> external
 * 
 */

// Don't use this directly. It's for internal use.
typedef struct {
  uint32_t len;
  union {
    char inline_data[INLINE_VEC_BYTES];
    struct {
      uint32_t cap;
      void *data;
    };
  };
} vec_void;

#define VEC_NEW { .len = 0, .data = NULL }

#define VEC_GET(vec, i) VEC_DATA_PTR(&vec)[i]

void __vec_push(vec_void *vec, void *el, size_t elemsize);

#define VEC_PUSH(vec, el) { \
  debug_assert(sizeof((vec)->data[0]) == sizeof(el)); \
  typeof(el) __el = el; \
  __vec_push((vec_void*) vec, (void*) &__el, sizeof(el)); \
}

vec_void *__vec_pop(vec_void *vec, size_t elemsize);

#define VEC_POP_(vec) \
    ((typeof(vec)) __vec_pop((vec_void*) vec, sizeof((vec)->inline_data[0])))

#define VEC_POP(vec) \
  ({ \
    typeof((vec)->inline_data[0]) __el = VEC_PEEK(*vec); \
    VEC_POP_(vec); \
    __el; \
  })

#define VEC_PEEK(vec) VEC_GET(vec, (vec).len - 1)

#define VEC_PEEK_REF(vec) VEC_GET(*(vec), (vec)->len - 1)
  
#define VEC_BACK(vec) VEC_PEEK(vec)

#define VEC_FREE(vec) if (VEC_IS_EXTERNAL(vec) && (vec)->data != NULL) { free((vec)->data); (vec)->data = NULL; }

void __vec_clone(vec_void *dest, vec_void *src, size_t elemsize);
#define VEC_CLONE(dest, src) { \
    debug_assert(sizeof((dest)->data[0]) == sizeof((src)->data[0])); \
    __vec_clone((vec_void*) (dest), (vec_void*) (src), sizeof((src)->data[0])); \
  }

// returns minimum heap-allocated buffer
#define VEC_FINALIZE(vec) __vec_finalize((vec_void*) vec, sizeof((vec)->data[0]))

void __vec_append(vec_void *vec, void *els, size_t amt, size_t elemsize);

#define VEC_APPEND(vec, amt, els) { \
    debug_assert(sizeof((vec)->data[0]) == sizeof((els)[0])); \
    debug_assert(amt == 0 || els != NULL); \
    __vec_append((vec_void*) vec, (void*) (els), amt, sizeof((els)[0])); \
  }

void __vec_replicate(vec_void *vec, void *el, size_t amt, size_t elemsize);

// appends el to vec amt times
#define VEC_REPLICATE(vec, amt, el) { \
    debug_assert(sizeof((vec)->len) == sizeof(amt)); \
    debug_assert(sizeof((vec)->data[0]) == sizeof(el)); \
    typeof(el) __el = el; \
    __vec_replicate((vec_void*) vec, (void*) &__el, amt, sizeof(el)); \
  }

#define VEC_CAT(v1, v2) \
  VEC_APPEND((v1), (v2)->len, VEC_DATA_PTR(v2))

typedef char* string;

VEC_DECL(string);