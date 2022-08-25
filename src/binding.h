#pragma once

#include "consts.h"
#include "vec.h"
#include "span.h"

#define binding span

VEC_DECL_CUSTOM(binding, vec_binding);

typedef union {
  char *builtin;
  binding span;
} str_ref;

VEC_DECL(str_ref);
