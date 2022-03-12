#include "vec.h"
#include "util.h"

#define VEC_RESIZE(vec, _cap)                                                  \
  __vec_resize((vec_void *)vec, _cap, sizeof((vec)->data[0]))

#define VEC_GROW(vec, _cap)                                                    \
  __vec_grow((vec_void *)vec, _cap, sizeof((vec)->data[0]))

#if INLINE_VEC_BYTES > 0
static void __vec_resize_internal_to_external(vec_void *vec, size_t cap,
                                              size_t elemsize) {
  // was inline, now external
  char *data = malloc(cap * elemsize);
  debug_assert(data != NULL);
  memcpy(data, &vec->inline_data, elemsize * MIN(vec->len, cap));
  vec->data = data;
  vec->cap = cap;
  vec->len = MIN(vec->len, cap);
}
#endif

static void __vec_resize_external_to_external(vec_void *vec, size_t cap,
                                              size_t elemsize) {
  // was external, still external
  vec->data = realloc(vec->data, elemsize * cap);
  debug_assert(vec->data != NULL);
  vec->cap = cap;
  vec->len = MIN(vec->len, cap);
}

static void __vec_resize_null_to_external(vec_void *vec, size_t cap,
                                          size_t elemsize) {
  // was external, still external
  vec->data = malloc(elemsize * cap);
  debug_assert(vec->data != NULL);
  vec->cap = cap;
  vec->len = MIN(vec->len, cap);
}

#if INLINE_VEC_BYTES > 0
static void __vec_resize_external_to_internal(vec_void *vec, size_t cap,
                                              size_t elemsize) {
  // was external, now internal
  char *data = vec->data;
  memcpy(&vec->inline_data, data, elemsize * MIN(vec->len, cap));
  free(data);
}
#endif

void __vec_push(vec_void *vec, void *el, size_t elemsize) {
#if INLINE_VEC_BYTES > 0
  const size_t inline_amt = SIZE_TO_INLINE_AMT(elemsize);
  if (vec->len > inline_amt) {
    // we're external
    // predicated this way for the branch predictor
    if (vec->len < vec->cap) {
    } else {
      __vec_resize_external_to_external(
        vec, MAX((vec->len + 1) * 2, VEC_FIRST_SIZE), elemsize);
    }
    memcpy(((char *)vec->data) + elemsize * vec->len, el, elemsize);
  } else {
    // we're internal
    if (vec->len == inline_amt) {
      // going external
      __vec_resize_internal_to_external(
        vec, MAX((vec->len + 1) * 2, VEC_FIRST_SIZE), elemsize);
      memcpy(((char *)vec->data) + elemsize * vec->len, el, elemsize);
    } else {
      memcpy(vec->inline_data + elemsize * vec->len, el, elemsize);
    }
  }
#else
  // predicated this way for the branch predictor
  if (vec->cap > vec->len) {
  } else if (vec->cap == 0) {
    __vec_resize_null_to_external(vec, MAX(VEC_FIRST_SIZE, 1), elemsize);
  } else {
    __vec_resize_external_to_external(
      vec, MAX(VEC_FIRST_SIZE, (vec->len + 1) * 2), elemsize);
  }
  memcpy(vec->data + elemsize * vec->len, el, elemsize);
#endif
  vec->len++;
}

void __vec_append(vec_void *vec, void *els, size_t amt, size_t elemsize) {

#if INLINE_VEC_BYTES > 0
  const size_t inline_amt = SIZE_TO_INLINE_AMT(elemsize);
  if (vec->len > inline_amt) {
    // we're external
    // predicated this way for the branch predictor
    if (vec->len + amt <= vec->cap) {
    } else {
      __vec_resize_external_to_external(
        vec, MAX((vec->len + amt) * 2, VEC_FIRST_SIZE), elemsize);
    }
    memcpy(((char *)vec->data) + elemsize * vec->len, els, elemsize * amt);
  } else {
    // we're internal
    if (vec->len + amt > inline_amt) {
      // going external
      __vec_resize_internal_to_external(
        vec, MAX((vec->len + amt) * 2, VEC_FIRST_SIZE), elemsize);
      memcpy(((char *)vec->data) + elemsize * vec->len, els, elemsize * amt);
    } else {
      memcpy(vec->inline_data + elemsize * vec->len, els, elemsize * amt);
    }
  }
#else
  // predicated this way for the branch predictor
  if (vec->cap >= vec->len + amt) {
  } else if (vec->cap == 0) {
    __vec_resize_null_to_external(vec, MAX(VEC_FIRST_SIZE, amt), elemsize);
  } else {
    __vec_resize_external_to_external(
      vec, MAX(VEC_FIRST_SIZE, (vec->len + amt) * 2), elemsize);
  }
  memcpy(vec->data + elemsize * vec->len, els, elemsize * amt);

#endif
  vec->len += amt;
}

void __vec_replicate(vec_void *vec, void *el, size_t amt, size_t elemsize) {

#if INLINE_VEC_BYTES > 0
  const size_t inline_amt = SIZE_TO_INLINE_AMT(elemsize);
  if (vec->len > inline_amt) {
    // we're external
    // predicated this way for the branch predictor
    if (vec->len + amt <= vec->cap) {
    } else {
      __vec_resize_external_to_external(
        vec, MAX((vec->len + amt) * 2, VEC_FIRST_SIZE), elemsize);
    }
    memset_arbitrary((char *)vec->data + elemsize * vec->len, el, amt,
                     elemsize);
  } else {
    // we're internal
    if (vec->len + amt > inline_amt) {
      // going external
      __vec_resize_internal_to_external(
        vec, MAX((vec->len + amt) * 2, VEC_FIRST_SIZE), elemsize);
      memset_arbitrary((char *)vec->data + elemsize * vec->len, el, amt,
                       elemsize);
    } else {
      memset_arbitrary(vec->inline_data + elemsize * vec->len, el, amt,
                       elemsize);
    }
  }
#else
  // predicated this way for the branch predictor
  if (vec->cap >= vec->len + amt) {
  } else if (vec->cap == 0) {
    __vec_resize_null_to_external(vec, MAX(VEC_FIRST_SIZE, amt), elemsize);
  } else {
    __vec_resize_external_to_external(
      vec, MAX(VEC_FIRST_SIZE, (vec->len + amt) * 2), elemsize);
  }
  memset_arbitrary(vec->data + elemsize * vec->len, el, amt, elemsize);
#endif

  vec->len += amt;
}

#if INLINE_VEC_BYTES > 0
vec_void *__vec_pop(vec_void *vec, size_t elemsize) {
  debug_assert(vec->len > 0);
  const size_t inline_amt = SIZE_TO_INLINE_AMT(elemsize);
  // predicated this way for the branch predictor
  if (vec->len - 1 != inline_amt) {
  } else {
    __vec_resize_external_to_internal(vec, vec->len - 1, elemsize);
  }
  vec->len--;
  return vec;
}
#else
vec_void *__vec_pop(vec_void *vec) {
  debug_assert(vec->len > 0);
  vec->len--;
  return vec;
}
#endif

// returns minimum heap-allocated buffer
char *__vec_finalize(vec_void *vec, size_t elemsize) {
  const size_t inline_amt = SIZE_TO_INLINE_AMT(elemsize);
#if INLINE_VEC_BYTES > 0
  if (vec->len <= inline_amt) {
    __vec_resize_internal_to_external(vec, vec->len, elemsize);
  }
#endif
  return vec->data;
}

void __vec_clone(vec_void *dest, vec_void *src, size_t elemsize) {
  dest->len = src->len;
#if INLINE_VEC_BYTES > 0
  const size_t inline_amt = SIZE_TO_INLINE_AMT(elemsize);
  if (src->len <= inline_amt) {
    memcpy(dest->inline_data, src->inline_data, elemsize * src->len);
  } else {
    dest->data = memclone(src->data, elemsize * src->len);
    dest->cap = src->len;
  }

#else
  dest->data = memclone(src->data, elemsize * src->len);
  dest->cap = src->len;
#endif
}
