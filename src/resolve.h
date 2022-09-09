#pragma once

#include "consts.h"
#include "ir.h"
#include "vec.h"

typedef enum {
  FUN_NODE,
  LET_NODE,
} sig_err_type;

typedef struct {
  enum {
    SIG_MISMATCH,
    SIG_WITHOUT_BINDING,
    BINDING_WITHOUT_SIG,
  } tag;

  union {
    struct {
      sig_err_type node_type;
      node_ind_t node_ind;
    };
  };
} resolve_error;

VEC_DECL(resolve_error);

typedef struct {
  ir_module module;
  resolve_error *errors;
  size_t error_amt;
} resolve_res;
