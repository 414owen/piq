#include <string.h>

#include "util.h"

void *memclone(void *src, size_t bytes) {
  void *dest = malloc(bytes);
  memcpy(dest, src, bytes);
  return dest;
}

char *join(size_t str_amt, char **strs) {
  size_t space = 0;
  size_t *lens = __builtin_alloca(str_amt * sizeof(size_t));
  for (size_t i = 0; i < str_amt; i++) {
    lens[i] = strlen(strs[i]);
    space += lens[i];
  }
  char *res = malloc(space + 1);
  res[space] = '\0';
  size_t ind = 0;
  for (size_t i = 0; i < str_amt; i++) {
    memcpy(res + ind, strs[i], lens[i]);
    ind += lens[i];
  }
  return res;
}
