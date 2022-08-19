#pragma once

#include "consts.h"
#include "vec.h"

typedef struct {
  buf_ind_t start;
  buf_ind_t end;
} binding;

VEC_DECL(binding);

typedef union {
  char *builtin;
  struct {
    buf_ind_t start;
    buf_ind_t end;
  };
} str_ref;

VEC_DECL(str_ref);
