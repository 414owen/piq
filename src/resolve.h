#pragma once

/* THIS MODULE IS NOT USED */

#include "consts.h"
#include "ir.h"
#include "vec.h"

typedef struct {
  enum {
    SIG_MISMATCH,
    SIG_WITHOUT_BINDING,
    BINDING_WITHOUT_SIG,
  } tag;

  node_ind_t node_ind;
} resolve_error;

VEC_DECL(resolve_error);

typedef struct {
  ir_module module;
  resolve_error *errors;
  size_t error_amt;
} resolve_res;
