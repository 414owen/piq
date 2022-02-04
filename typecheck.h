#pragma once

#include "parse_tree.h"
#include "vec.h"
#include "types.h"

typedef struct {
  enum {
    BINDING_NOT_FOUND,
    TYPE_MISMATCH,
    WRONG_ARITY,
  } err_type;

  NODE_IND_T pos;

  union {
    struct {
      type expected;
      type got;
    };
    struct {
      NODE_IND_T exp_param_amt;
      NODE_IND_T got_param_amt;
    };
  };

} tc_error;

VEC_DECL(tc_error);

typedef struct {
  bool successful;
  vec_tc_error errors;
  vec_type types;
} tc_res;