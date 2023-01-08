#include "vec.h"
#include "util.h"

#if INLINE_VEC_BYTES > 0
static void __vec_resize_internal_to_external(vec_void *vec, VEC_VEC_LEN_T cap,
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

static void __vec_resize_external_to_external(vec_void *vec, VEC_LEN_T cap,
                                              size_t elemsize) {
  // was external, still external
  vec->data = realloc(vec->data, elemsize * cap);
  debug_assert(vec->data != NULL);
  vec->cap = cap;
  vec->len = MIN(vec->len, cap);
}

static void __vec_resize_null_to_external(vec_void *vec, VEC_LEN_T cap,
                                          size_t elemsize) {
  // was external, still external
  vec->data = malloc(elemsize * cap);
  debug_assert(vec->data != NULL);
  vec->cap = cap;
  vec->len = MIN(vec->len, cap);
}

#if INLINE_VEC_BYTES > 0
static void __vec_resize_external_to_internal(vec_void *vec, VEC_LEN_T cap,
                                              size_t elemsize) {
  // was external, now internal
  char *data = vec->data;
  memcpy(&vec->inline_data, data, elemsize * MIN(vec->len, cap));
  free(data);
}
#endif

#ifdef DEBUG
void debug_vec_get(vec_void *vec, VEC_LEN_T ind) {
  if (ind >= vec->len) {
    give_up("Tried to access invalid vector element");
  }
}
void debug_vec_get_ptr(vec_void *vec, VEC_LEN_T ind) {
  if (ind > 0 && ind >= vec->len) {
    give_up("Tried to access invalid vector element");
  }
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
  if (HEDLEY_PREDICT_FALSE(vec->cap <= vec->len, 0.9)) {
    __vec_resize_external_to_external(
      vec, MAX(VEC_FIRST_SIZE, (vec->len + 1) * 2), elemsize);
  } else if (vec->cap == 0) {
    __vec_resize_null_to_external(vec, MAX(VEC_FIRST_SIZE, 1), elemsize);
  }
  memcpy(((char *)vec->data) + elemsize * vec->len, el, elemsize);
#endif
  vec->len++;
}

void __vec_append(vec_void *restrict vec, void *restrict els, VEC_LEN_T amt,
                  size_t elemsize) {

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
  memcpy(((char *)vec->data) + elemsize * vec->len, els, elemsize * amt);

#endif
  vec->len += amt;
}

void __vec_append_reverse(vec_void *restrict vec, void *restrict els,
                          VEC_LEN_T amt, size_t elemsize) {
  __vec_append(vec, els, amt, elemsize);
  char *start = ((char *)VEC_DATA_PTR(vec)) + elemsize * (vec->len - amt);
  char *tmp = stalloc(elemsize);
  for (VEC_LEN_T i = 0; i < amt / 2; i++) {
    char *a = start + elemsize * i;
    char *b = start + (amt - 1 - i) * elemsize;
    memcpy(tmp, a, elemsize);
    memcpy(a, b, elemsize);
    memcpy(b, tmp, elemsize);
  }
  stfree(tmp, elemsize);
}

void __vec_replicate(vec_void *vec, void *el, VEC_LEN_T amt, size_t elemsize) {

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
    memset_arbitrary(
      (char *)vec->data + elemsize * vec->len, el, amt, elemsize);
  } else {
    // we're internal
    if (vec->len + amt > inline_amt) {
      // going external
      __vec_resize_internal_to_external(
        vec, MAX((vec->len + amt) * 2, VEC_FIRST_SIZE), elemsize);
      memset_arbitrary(
        (char *)vec->data + elemsize * vec->len, el, amt, elemsize);
    } else {
      memset_arbitrary(
        vec->inline_data + elemsize * vec->len, el, amt, elemsize);
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
  memset_arbitrary(((char*) vec->data) + elemsize * vec->len, el, amt, elemsize);
#endif

  vec->len += amt;
}

#if INLINE_VEC_BYTES > 0
void __vec_pop_n(vec_void *vec, size_t elemsize, VEC_LEN_T n) {
  debug_assert(vec->len >= n);
  const size_t inline_amt = SIZE_TO_INLINE_AMT(elemsize);
  if (vec->len > inline_amt && vec->len - n <= inline_amt) {
    __vec_resize_external_to_internal(vec, vec->len - n, elemsize);
  }
  vec->len -= n;
}
#else
void __vec_pop_n(vec_void *vec, VEC_LEN_T n) {
  debug_assert(vec->len >= n);
  vec->len -= n;
}
#endif

#if INLINE_VEC_BYTES > 0
vec_void *__vec_pop(vec_void *vec, size_t elemsize) {
  return __vec_pop_n(vec, elemsize, 1);
}
#else
void __vec_pop(vec_void *vec) { __vec_pop_n(vec, 1); }
#endif

static void zero_vector(vec_void *vec) {
  const vec_void a = VEC_NEW;
  *vec = a;
}

#if INLINE_VEC_BYTES > 0

// This is dumb, finalize should just shrink the vector.
// This should be called something else.
char *__vec_finalize(vec_void *vec, size_t elemsize) {
  const size_t inline_amt = SIZE_TO_INLINE_AMT(elemsize);
  if (vec->len == 0 && vec->data != NULL) {
    free(vec->data);
    return NULL;
  }
  if (vec->len <= inline_amt) {
    __vec_resize_internal_to_external(vec, vec->len, elemsize);
  }
  char *res = vec->data;
  zero_vector(vec);
  return res;
}

#else

char *__vec_finalize(vec_void *vec, size_t elemsize) {
  if (vec->len == 0 && vec->data != NULL) {
    free(vec->data);
    vec->cap = 0;
    return NULL;
  }
  void *res =
    vec->len == vec->cap ? vec->data : realloc(vec->data, elemsize * vec->len);
  zero_vector(vec);
  return res;
}

#endif

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

#if INLINE_VEC_BYTES > 0
void __vec_reserve(vec_void *vec, VEC_LEN_T amt, size_t elemsize) {
  VEC_LEN_T len = vec->len;
  VEC_LEN_T req = vec->len + amt;
  const size_t inline_amt = SIZE_TO_INLINE_AMT(elemsize);
  if (len <= inline_amt) {
    if (req <= inline_amt) {
      return;
    }
    __vec_resize_internal_to_external(vec, req, elemsize);
  }
  if (vec->cap < req) {
    __vec_resize_external_to_external(vec, req, elemsize);
  }
}
#else
void __vec_reserve(vec_void *vec, VEC_LEN_T amt, size_t elemsize) {
  __vec_resize_external_to_external(vec, vec->len + amt, elemsize);
}
#endif
