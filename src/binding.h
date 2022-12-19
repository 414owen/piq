#pragma once

#include "consts.h"
#include "vec.h"
#include "span.h"

typedef span binding;

VEC_DECL_CUSTOM(binding, vec_binding);

typedef union {
  const char *builtin;
  binding binding;
} str_ref;

VEC_DECL(str_ref);
