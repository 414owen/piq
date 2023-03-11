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

order compare_bnds(const char *restrict source_file, binding a, binding b) {
  return strncmp(
    source_file + a.start, source_file + b.start, MIN(a.len, b.len));
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

// TODO remove? not currently used AFAIK
size_t lookup_bnd(const char *source_file, binding *bnds, node_ind_t bnd_amt,
                  binding bnd) {
  const char *bndp = source_file + bnd.start;
  for (size_t i = 0; i < bnd_amt; i++) {
    size_t ind = bnd_amt - 1 - i;
    binding a = bnds[ind];
    if (strncmp(bndp, source_file + a.start, bnd.len) == 0) {
      return ind;
    }
  }
  return bnd_amt;
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

/*
scopes_builder scopes_new_builder(const char *restrict input) {
  scopes_builder res = {
    .input = input,
    .scopes = VEC_NEW,
  };
  return res;
}

scoped_binding_ref scopes_lookup_binding(const scopes_builder *restrict scopes,
binding bnd) { vec_scope_builder builders = scopes->scopes; scope_ref scope_amt
= builders.len; if (scope_amt == 0) { goto no_results;
  }
  scope_ref scope_ind = builders.len - 1;
  scope_builder scope = VEC_GET(builders, scope_ind);
  while (true) {
    for (binding_ref i = 0; i < scope.bindings.len; i++) {
      binding_ref j = scope.bindings.len - 1 - i;
      binding b = VEC_GET(scope.bindings, j);
      if (compare_bnds(scopes->input, b, bnd) == 0) {
        scoped_binding_ref res = {
          .scope_ref = scope_ind,
          .binding_ind = j,
        };
        return res;
      }
    }
    if (scope_ind == 0) {
      goto no_results;
    }
    scope_ind = scope.parent;
    VEC_GET(builders, scope_ind);
  }
  no_results: {
    scoped_binding_ref res = {
      .scope_ref = 0,
      .binding_ind = 0,
    };
    return res;
  }
}

void scopes_add_binding(scopes_builder *restrict scopes, binding bnd) {
  scope_builder *scope = VEC_PEEK_PTR(scopes->scopes);
  VEC_PUSH(&scope->bindings, bnd);
}

scope_ref scopes_push(scopes_builder *restrict scopes) {
  scope_builder scope = {
    .bindings = VEC_NEW,
    .parent = scopes->scopes.len,
  };
  VEC_PUSH(&scopes->scopes, scope);
  return scopes->scopes.len - 1;
}

// popping doesn't remove anything from the builder
// it just adds a new scope with its parent the same as the current scope's
parent scope_ref scopes_pop(scopes_builder *restrict scopes) { scope_builder
scope = { .bindings = VEC_NEW, .parent = VEC_PEEK(scopes->scopes).parent,
  };
  VEC_PUSH(&scopes->scopes, scope);
  return scopes->scopes.len - 1;
}
*/
