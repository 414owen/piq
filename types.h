#pragma once

#include "parse_tree.h"
#include "vec.h"

typedef enum {
  T_UNKNOWN,
  T_I8, T_U8, T_I16, T_U16, T_I32, T_U32, T_I64, T_U64,
  T_FN, T_BOOL, T_TUP, T_LIST
} type_tag;

typedef struct {
  type_tag tag;
  NODE_IND_T sub_start;
  NODE_IND_T sub_amt;
} type;

void print_type(FILE *f, type *types, NODE_IND_T *inds, NODE_IND_T root);

VEC_DECL(type);
