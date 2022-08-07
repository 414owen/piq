#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "util.h"

typedef uint8_t u8;
typedef uint32_t u32;

#define VEC_LEN_T u32
#define VEC_FIRST_SIZE 10

#ifndef INLINE_VEC_BYTES
#define INLINE_VEC_BYTES 0
#endif

#define SIZE_TO_INLINE_AMT(size) (INLINE_VEC_BYTES / size)
#define TYPE_TO_INLINE_AMT(type) (SIZE_TO_INLINE_AMT(sizeof(type)))

#if INLINE_VEC_BYTES > 0
#define VEC_IS_INLINE(vec) ((vec)->len <= TYPE_TO_INLINE_AMT((vec)->data[0]))
#define VEC_IS_EXTERNAL(vec) ((vec)->len > TYPE_TO_INLINE_AMT((vec)->data[0]))
#define VEC_DATA_PTR(vec)                                                      \
  (VEC_IS_EXTERNAL(vec) ? (vec)->data : (vec)->inline_data)
#else
#define VEC_IS_INLINE(vec) false
#define VEC_IS_EXTERNAL(vec) true
#define VEC_DATA_PTR(vec) ((vec)->data)
#endif

#if INLINE_VEC_BYTES > 0

#define VEC_DECL(type)                                                         \
  typedef struct {                                                             \
    VEC_LEN_T len;                                                             \
    union {                                                                    \
      type inline_data[TYPE_TO_INLINE_AMT(type)];                              \
      struct {                                                                 \
        VEC_LEN_T cap;                                                         \
        type *data;                                                            \
      };                                                                       \
    };                                                                         \
  } vec_##type

#else

#define VEC_DECL(type)                                                         \
  typedef struct {                                                             \
    VEC_LEN_T len;                                                             \
    struct {                                                                   \
      VEC_LEN_T cap;                                                           \
      type *data;                                                              \
    };                                                                         \
  } vec_##type

#endif

/*
 * len > inline_amt -> external
 * len <= inline_amt -> external
 *
 */

// Don't use this directly. It's for internal use.
typedef struct {
  VEC_LEN_T len;
#if INLINE_VEC_BYTES > 0
  union {
    char inline_data[INLINE_VEC_BYTES];
    struct {
      VEC_LEN_T cap;
      void *data;
    };
  };
#else
  struct {
    VEC_LEN_T cap;
    void *data;
  };
#endif
} vec_void;

#define VEC_NEW                                                                \
  { .len = 0, .cap = 0, .data = NULL }

#define VEC_GET_DIRECT(vec, i) (VEC_DATA_PTR(&(vec))[i])

#ifdef DEBUG
void debug_vec_get(vec_void *vec, size_t elemsize, VEC_LEN_T ind);
// Avoid duplicate side-effects in i (we were using VEC_POP()
#define VEC_GET(vec, i)                                                        \
  ({                                                                           \
    size_t __ind = (i);                                                        \
    debug_vec_get((vec_void *)(&vec), sizeof(vec.data[0]), __ind);             \
    VEC_GET_DIRECT(vec, __ind);                                                \
  })
#else
#define VEC_GET(vec, i) VEC_GET_DIRECT(vec, i)
#endif

#define VEC_GET_PTR(vec, i) (&VEC_GET_DIRECT(vec, i))

void __vec_push(vec_void *vec, void *el, size_t elemsize);
#define VEC_PUSH(vec, el)                                                      \
  {                                                                            \
    debug_assert(sizeof((vec)->data[0]) == sizeof(el));                        \
    typeof(el) __el = el;                                                      \
    __vec_push((vec_void *)vec, (void *)&__el, sizeof(el));                    \
  }

#if INLINE_VEC_BYTES > 0
vec_void *__vec_pop(vec_void *vec, size_t elemsize);
#define VEC_POP_(vec)                                                          \
  ((typeof(vec))__vec_pop((vec_void *)vec, sizeof((vec)->data[0])))
#else
vec_void *__vec_pop(vec_void *vec);
#define VEC_POP_(vec) ((typeof(vec))__vec_pop((vec_void *)vec))
#endif

#if INLINE_VEC_BYTES > 0
vec_void *__vec_pop_n(vec_void *vec, size_t elemsize, VEC_LEN_T n);
#define VEC_POP_N(vec, n)                                                      \
  ((typeof(vec))__vec_pop_n((vec_void *)vec, sizeof((vec)->data[0]), n))
#else
vec_void *__vec_pop_n(vec_void *vec, VEC_LEN_T n);
#define VEC_POP_N(vec, n) ((typeof(vec))__vec_pop_n((vec_void *)vec, n))
#endif

#define VEC_POP(vec)                                                           \
  ({                                                                           \
    typeof((vec)->data[0]) __el = VEC_PEEK(*vec);                              \
    VEC_POP_(vec);                                                             \
    __el;                                                                      \
  })

#define VEC_PEEK(vec) VEC_GET(vec, (vec).len - 1)

#define VEC_PEEK_REF(vec) VEC_GET(*(vec), (vec)->len - 1)

#define VEC_BACK(vec) VEC_PEEK(vec)

#define VEC_FIRST(vec) VEC_GET((vec), 0)

#define VEC_LAST(vec) VEC_GET((vec), (vec).len - 1)

#define VEC_FREE(vec)                                                          \
  if (VEC_IS_EXTERNAL(vec) && (vec)->data != NULL) {                           \
    free((vec)->data);                                                         \
    (vec)->data = NULL;                                                        \
  }

void __vec_clone(vec_void *dest, vec_void *src, size_t elemsize);
#define VEC_CLONE(dest, src)                                                   \
  {                                                                            \
    debug_assert(sizeof((dest)->data[0]) == sizeof((src)->data[0]));           \
    __vec_clone((vec_void *)(dest), (vec_void *)(src),                         \
                sizeof((src)->data[0]));                                       \
  }

#if INLINE_VEC_BYTES > 0

char *__vec_finalize(vec_void *vec, size_t elemsize);
// returns minimum heap-allocated buffer
#define VEC_FINALIZE(vec)                                                      \
  (typeof(*(vec.data)))__vec_finalize((vec_void *)vec, sizeof((vec)->data[0]))

#else

char *__vec_finalize(vec_void *vec);
// returns minimum heap-allocated buffer
#define VEC_FINALIZE(vec) (typeof(*(vec.data)))__vec_finalize((vec_void *)vec)

#endif

void __vec_append(vec_void *vec, void *els, VEC_LEN_T amt, size_t elemsize);

#define VEC_APPEND(vec, amt, els)                                              \
  {                                                                            \
    debug_assert(sizeof((vec)->data[0]) == sizeof((els)[0]));                  \
    debug_assert(amt == 0 || els != NULL);                                     \
    __vec_append((vec_void *)vec, (void *)(els), amt, sizeof((els)[0]));       \
  }

void __vec_replicate(vec_void *vec, void *el, VEC_LEN_T amt, size_t elemsize);

// appends el to vec amt times
#define VEC_REPLICATE(vec, amt, el)                                            \
  {                                                                            \
    debug_assert(sizeof((vec)->len) == sizeof(amt));                           \
    debug_assert(sizeof((vec)->data[0]) == sizeof(el));                        \
    typeof(el) __el = el;                                                      \
    __vec_replicate((vec_void *)vec, (void *)&__el, amt, sizeof(el));          \
  }

#define VEC_CAT(v1, v2) VEC_APPEND((v1), (v2)->len, VEC_DATA_PTR(v2))

#define VEC_CLEAR(v) ((v)->len = 0)

typedef char *string;

VEC_DECL(string);

VEC_DECL(u8);
VEC_DECL(u32);
VEC_DECL(char);