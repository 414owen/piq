#include <stdbool.h>
#include <stdlib.h>

#include "binding.h"
#include "bitset.h"
#include "scope.h"

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

// TODO consider separating builtins into separate array somehow
// Find index of binding, return bindings.len if not found
size_t lookup_bnd(const char *source_file, vec_str_ref bnds, bitset is_builtin,
                  binding bnd) {
  size_t len = bnd.end - bnd.start + 1;
  const char *bndp = source_file + bnd.start;
  for (size_t i = 0; i < bnds.len; i++) {
    size_t ind = bnds.len - 1 - i;
    str_ref a = bnds.data[ind];
    if (bs_get(is_builtin, ind)) {
      if (strn1eq(bndp, a.builtin, len)) {
        return ind;
      }
    } else {
      if (a.end - a.start != len)
        continue;
      if (strncmp(bndp, source_file + a.start, len) == 0)
        return ind;
    }
  }
  return bnds.len;
}
