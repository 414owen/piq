#include "vec.h"
#include "util.h"

void __vec_resize(vec_void *vec, size_t cap, size_t elemsize) {
  vec->data = realloc(vec->data, cap * elemsize);
  assert(vec->data != NULL);
  vec->cap = cap;
}

void __vec_grow(vec_void *vec, size_t cap, size_t elemsize) {
  if (vec->cap < cap)
    __vec_resize(vec, MAX(cap, 10), elemsize);
}

void __vec_push(vec_void *vec, void *el, size_t elemsize) {
  __vec_grow(vec, vec->len + 1, elemsize);
  // SPEEDUP: Probably faster to do this one by hand
  memcpy(((char *)vec->data) + elemsize * vec->len, el, elemsize);
  vec->len++;
}

void __vec_append(vec_void *vec, void *els, size_t amt, size_t elemsize) {
  __vec_grow(vec, vec->len + amt, elemsize);
  memcpy(((char *)vec->data) + elemsize * vec->len, els, amt * elemsize);
  vec->len += amt;
}

void __vec_replicate(vec_void *vec, void *el, size_t amt, size_t elemsize) {
  __vec_grow(vec, vec->len + amt, elemsize);
  memset_arbitrary(((char *)vec->data) + elemsize * vec->len, el, amt,
                   elemsize);
  vec->len += amt;
}
