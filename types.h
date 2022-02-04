#pragma once

#include "parse_tree.h"
#include "vec.h"

typedef enum {
  T_UNKNOWN,
  T_I8, T_U8, T_I16, T_U16, T_I32, T_U32, T_I64, T_U64,
  T_FN,
} type_tag;

typedef struct {
  type_tag tag;
  NODE_IND_T sub_start;
  NODE_IND_T sub_amt;
} type;

VEC_DECL(type);
