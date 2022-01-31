#pragma once

#include "parse_tree.h"
#include "vec.h"

typedef enum {
  T_UNKNOWN,
  T_I8, T_U8, T_I16, T_U16, T_I32, T_U32, T_I64, T_U64,
} type;

VEC_DECL(type);

typedef struct {
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