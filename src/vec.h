// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "consts.h"
#include "util.h"
#include "typedefs.h"

#define VEC_LEN_T u32
#define VEC_FIRST_SIZE 8

#ifndef INLINE_VEC_BYTES
#define INLINE_VEC_BYTES 0
#endif

#define VEC_ELSIZE(vec) sizeof((vec)->data[0])
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

#define VEC_DECL_CUSTOM(type, name)                                            \
  typedef struct {                                                             \
    VEC_LEN_T len;                                                             \
    union {                                                                    \
      type inline_data[TYPE_TO_INLINE_AMT(type)];                              \
      struct {                                                                 \
        VEC_LEN_T cap;                                                         \
        type *data;                                                            \
      };                                                                       \
    };                                                                         \
  } name

#else

#define VEC_DECL_CUSTOM(type, name)                                            \
  typedef struct {                                                             \
    VEC_LEN_T len;                                                             \
    VEC_LEN_T cap;                                                             \
    type *data;                                                                \
  } name
#endif

#define VEC_DECL(type) VEC_DECL_CUSTOM(type, vec_##type)

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
  VEC_LEN_T cap;
  void *data;
#endif
} vec_void;

#define VEC_NEW                                                                \
  { .len = 0, .cap = 0, .data = NULL }

#define VEC_GET_DIRECT(vec, i) (VEC_DATA_PTR(&(vec))[i])

#ifndef NDEBUG
void debug_vec_get(vec_void *vec, VEC_LEN_T ind);
#define VEC_GET(vec, i)                                                        \
  (debug_vec_get((vec_void *)(&vec), (size_t)(i)), VEC_GET_DIRECT(vec, (i)))
#else
#define VEC_GET(vec, i) VEC_GET_DIRECT(vec, i)
#endif

#define VEC_GET_PTR(vec, i) (&VEC_GET_DIRECT(vec, (i)))

#define VEC_SET(vec, i, val) *(VEC_GET_PTR((vec), (i))) = (val)

void __vec_push(vec_void *vec, void *el, size_t elemsize);
#define VEC_PUSH(vec, el)                                                      \
  {                                                                            \
    debug_assert(VEC_ELSIZE(vec) == sizeof(el));                               \
    __typeof__(el) __el = el;                                                  \
    __vec_push((vec_void *)vec, (void *)&__el, sizeof(el));                    \
  }

#if INLINE_VEC_BYTES > 0
vec_void *__vec_pop(vec_void *vec, size_t elemsize);
#define VEC_POP_(vec)                                                          \
  ((__typeof__(vec))__vec_pop((vec_void *)vec, VEC_ELSIZE(vec)->data[0]))
#else
void __vec_pop(vec_void *vec);
#define VEC_POP_(vec) (__vec_pop((vec_void *)vec))
#endif

#if INLINE_VEC_BYTES > 0
void __vec_pop_n(vec_void *vec, size_t elemsize, VEC_LEN_T n);
#define VEC_POP_N(vec, n)                                                      \
  ((__typeof__(vec))__vec_pop_n((vec_void *)vec, VEC_ELSIZE(vec), n))
#else
void __vec_pop_n(vec_void *vec, VEC_LEN_T n);
#define VEC_POP_N(vec, n) (__vec_pop_n((vec_void *)vec, n))
#endif

#define VEC_POP(vec, elp)                                                      \
  *(elp) = VEC_PEEK(*vec);                                                     \
  VEC_POP_(vec);

#define VEC_PEEK(vec) VEC_GET((vec), (vec).len - 1)

#define VEC_PEEK_PTR(vec) VEC_GET_PTR((vec), (vec).len - 1)

#define VEC_BACK(vec) VEC_PEEK(vec)

#define VEC_FIRST(vec) VEC_GET((vec), 0)

#define VEC_LAST(vec) VEC_GET((vec), (vec).len - 1)

#define VEC_FREE(vec)                                                          \
  (vec)->len = 0;                                                              \
  (vec)->cap = 0;                                                              \
  if (VEC_IS_EXTERNAL(vec) && (vec)->data != NULL) {                           \
    free((vec)->data);                                                         \
    (vec)->data = NULL;                                                        \
  }

void __vec_clone(vec_void *dest, vec_void *src, size_t elemsize);
#define VEC_CLONE(dest, src)                                                   \
  {                                                                            \
    debug_assert(VEC_ELSIZE(dest) == sizeof((src)->data[0]));                  \
    __vec_clone((vec_void *)(dest), (vec_void *)(src), VEC_ELSIZE(src));       \
  }

char *__vec_finalize(vec_void *vec, size_t elemsize);

// returns minimum heap-allocated buffer
#define VEC_FINALIZE(vec)                                                      \
  (__typeof__(*(vec.data)))__vec_finalize((vec_void *)vec, VEC_ELSIZE(vec))

void __vec_append(vec_void *vec, void *els, VEC_LEN_T amt, size_t elemsize);

#define VEC_APPEND(vec, amt, els)                                              \
  {                                                                            \
    debug_assert(VEC_ELSIZE(vec) == sizeof((els)[0]));                         \
    debug_assert((amt) == 0 || (els) != NULL);                                 \
    __vec_append((vec_void *)(vec), (void *)(els), (amt), sizeof((els)[0]));   \
  }

#define VEC_REVERSE(vec)                                                       \
  reverse_arbitrary(VEC_DATA_PTR(vec), (vec)->len, VEC_ELSIZE(vec))

void __vec_append_reverse(vec_void *vec, void *els, VEC_LEN_T amt,
                          size_t elemsize);

#define VEC_APPEND_REVERSE(vec, amt, els)                                      \
  {                                                                            \
    debug_assert(VEC_ELSIZE(vec) == sizeof((els)[0]));                         \
    debug_assert(amt == 0 || els != NULL);                                     \
    __vec_append_reverse(                                                      \
      (vec_void *)vec, (void *)(els), amt, sizeof((els)[0]));                  \
  }

#define VEC_APPEND_STATIC(vec, els) VEC_APPEND((vec), STATIC_LEN(els), (els))

void __vec_replicate(vec_void *vec, void *el, VEC_LEN_T amt, size_t elemsize);

// appends el to vec amt times
#define VEC_REPLICATE(vec, amt, el)                                            \
  {                                                                            \
    debug_assert(VEC_ELSIZE(vec) == sizeof(el));                               \
    __typeof__(el) __el = el;                                                  \
    __vec_replicate((vec_void *)vec, (void *)&__el, amt, sizeof(el));          \
  }

void __vec_reserve(vec_void *vec, VEC_LEN_T amt, size_t elemsize);

/** Make sure the vector has at least this many *free* slots */
#define VEC_RESERVE(vec, amt)                                                  \
  __vec_reserve((vec_void *)vec, amt, VEC_ELSIZE(vec));

#define VEC_CAT(v1, v2) VEC_APPEND((v1), (v2)->len, VEC_DATA_PTR(v2))

#define VEC_CLEAR(v) ((v)->len = 0)

void *__vec_get_ptr(vec_void *vec, size_t elemsize, VEC_LEN_T ind);

typedef char *string;

VEC_DECL(string);

VEC_DECL(u8);
VEC_DECL(u32);
VEC_DECL(char);
VEC_DECL_CUSTOM(buf_ind_t, vec_buf_ind);
VEC_DECL_CUSTOM(environment_ind_t, vec_environment_ind);
