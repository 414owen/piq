#pragma once

#include "consts.h"
#include "vec.h"

typedef struct {
  BUF_IND_T start;
  BUF_IND_T end;
} binding;

VEC_DECL(binding);

typedef union {
  char *builtin;
  struct {
    BUF_IND_T start;
    BUF_IND_T end;
  };
} str_ref;

VEC_DECL(str_ref);
