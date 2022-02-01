#pragma once

#include "parse_tree.h"
#include "vec.h"
#include "types.h"

typedef struct {
  enum {
    BINDING_NOT_FOUND,
    TYPE_MISMATCH,
  } err_type;

  NODE_IND_T pos;

  type expected;
  type got;

} tc_error;

VEC_DECL(tc_error);

typedef struct {
  bool successful;
  vec_tc_error errors;
  vec_type types;
} tc_res;