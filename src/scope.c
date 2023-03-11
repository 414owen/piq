#include <stdbool.h>
#include <stdlib.h>

#include "binding.h"
#include "bitset.h"
#include "scope.h"
#include "util.h"

// For a known-length, and a null-terminated string
static bool strn1eq(const char *a, const char *b, size_t alen) {
  size_t i;
  for (i = 0; i < alen; i++) {
    // handles b '\0'
    if (a[i] != b[i])
      return false;
  }
  if (b[i] != '\0')
    return false;
  return true;
}

// This extremely simple string comparison improved compile time by about 3-4%
static bool streq(const char *restrict a, const char *restrict b,
                  buf_ind_t len) {
  while (len--) {
    if (*a != *b) {
      return false;
    }
    a++;
    b++;
  }
  return true;
}

// TODO consider separating builtins into separate array somehow
// Find index of binding, return bindings.len if not found
node_ind_t lookup_str_ref(const char *source_file, scope scope, binding bnd) {
  const char *bndp = source_file + bnd.start;
  for (size_t i = 0; i < scope.bindings.len; i++) {
    size_t ind = scope.bindings.len - 1 - i;
    str_ref a = VEC_GET(scope.bindings, ind);
    if (bs_get(scope.is_builtin, ind)) {
      if (strn1eq(bndp, a.builtin, bnd.len)) {
        return ind;
      }
    } else {
      if (a.binding.len != bnd.len)
        continue;
      if (streq(bndp, &source_file[a.binding.start], bnd.len))
        return ind;
    }
  }
  return scope.bindings.len;
}

scope scope_new(void) {
  scope res = {
    .bindings = VEC_NEW,
    .is_builtin = bs_new(),
  };
  return res;
}

void scope_push(scope *s, binding b) {
  str_ref str = {
    .binding = b,
  };
  bs_push_false(&s->is_builtin);
  VEC_PUSH(&s->bindings, str);
}

void scope_free(scope s) {
  bs_free(&s.is_builtin);
  VEC_FREE(&s.bindings);
}
