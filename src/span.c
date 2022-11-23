#include <stdbool.h>

#include "span.h"

bool spans_equal(span a, span b) {
  return a.start == b.start && a.len == b.len;
}
