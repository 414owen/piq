#include "vec.h"
#include "util.h"

#define VEC_RESIZE(vec, _cap) __vec_resize((vec_void*) vec, _cap, sizeof((vec)->data[0]))

#define VEC_GROW(vec, _cap) __vec_grow((vec_void*) vec, _cap, sizeof((vec)->data[0]))

static void __vec_resize_internal_to_external(vec_void *vec, size_t cap, size_t elemsize) {
  // was inline, now external
  char *data = malloc(cap * elemsize);
  debug_assert(data != NULL);
  memcpy(data, &vec->inline_data, elemsize * MIN(vec->len, cap));
  vec->data = data;
  vec->cap = cap;
  vec->len = MIN(vec->len, cap);
}

static void __vec_resize_external_to_external(vec_void *vec, size_t cap, size_t elemsize) {
  // was external, still external
  vec->data = realloc(vec->data, elemsize * cap);
  debug_assert(vec->data != NULL);
  vec->cap = cap;
  vec->len = MIN(vec->len, cap);
}

static void __vec_resize_external_to_internal(vec_void *vec, size_t cap, size_t elemsize) {
  // was external, now internal
  char *data = vec->data;
  memcpy(&vec->inline_data, data, elemsize * MIN(vec->len, cap));
  free(data);
}

void __vec_push(vec_void *vec, void *el, size_t elemsize) {
  const size_t inline_amt = SIZE_TO_INLINE_AMT(elemsize);
  if (vec->len > inline_amt) {
    // we're external
    // predicated this way for the branch predictor
    if (vec->len < vec->cap) {
    } else {
      __vec_resize_external_to_external(vec, MAX((vec->len + 1) * 2, VEC_FIRST_SIZE), elemsize);
    }
    memcpy(((char *)vec->data) + elemsize * vec->len, el, elemsize);
  } else {
    // we're internal
    if (vec->len == inline_amt) {
      // going external
      __vec_resize_internal_to_external(vec, MAX((vec->len + 1) * 2, VEC_FIRST_SIZE), elemsize);
      memcpy(((char *)vec->data) + elemsize * vec->len, el, elemsize);
    } else {
      memcpy(vec->inline_data + elemsize * vec->len, el, elemsize);
    }
  }
  vec->len++;
}

void __vec_append(vec_void *vec, void *els, size_t amt, size_t elemsize) {
  const size_t inline_amt = SIZE_TO_INLINE_AMT(elemsize);
  if (vec->len > inline_amt) {
    // we're external
    // predicated this way for the branch predictor
    if (vec->len + amt <= vec->cap) {
    } else {
      __vec_resize_external_to_external(vec, MAX((vec->len + amt) * 2, VEC_FIRST_SIZE), elemsize);
    }
    memcpy(((char *)vec->data) + elemsize * vec->len, els, elemsize * amt);
  } else {
    // we're internal
    if (vec->len + amt > inline_amt) {
      // going external
      __vec_resize_internal_to_external(vec, MAX((vec->len + amt) * 2, VEC_FIRST_SIZE), elemsize);
      memcpy(((char *)vec->data) + elemsize * vec->len, els, elemsize * amt);
    } else {
      memcpy(vec->inline_data + elemsize * vec->len, els, elemsize * amt);
    }
  }
  vec->len += amt;
}

void __vec_replicate(vec_void *vec, void *el, size_t amt, size_t elemsize) {
  const size_t inline_amt = SIZE_TO_INLINE_AMT(elemsize);
  if (vec->len > inline_amt) {
    // we're external
    // predicated this way for the branch predictor
    if (vec->len + amt <= vec->cap) {
    } else {
      __vec_resize_external_to_external(vec, MAX((vec->len + amt) * 2, VEC_FIRST_SIZE), elemsize);
    }
    memset_arbitrary((char *) vec->data + elemsize * vec->len, el, amt, elemsize);
  } else {
    // we're internal
    if (vec->len + amt > inline_amt) {
      // going external
      __vec_resize_internal_to_external(vec, MAX((vec->len + amt) * 2, VEC_FIRST_SIZE), elemsize);
      memset_arbitrary((char *) vec->data + elemsize * vec->len, el, amt, elemsize);
    } else {
      memset_arbitrary(vec->inline_data + elemsize * vec->len, el, amt, elemsize);
    }
  }
  vec->len += amt;
}
