#include <string.h>

#include "hash.h"
#include "source.h"

source_digest digest_from_string(const char *str) {
  source_digest res;
  res.length = str == NULL ? 0 : strlen(str);
  res.hash = hash(str, res.length);
  return res;
}
