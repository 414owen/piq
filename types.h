#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "parse_tree.h"
#include "vec.h"

typedef enum __attribute__((packed)) {
  T_UNKNOWN,
  T_UNIT,
  T_I8, T_U8, T_I16, T_U16, T_I32, T_U32, T_I64, T_U64,
  T_FN, T_BOOL, T_TUP, T_LIST
} type_tag;

// FN with arity 0 means sub_a and sub_b are present, and
// is equivalent to a CALL(CALL(FN, PARAM), RET)
//
// FN with arity 1 means sub_a is present, and
// is equivalent to a CALL(FN, PARAM)
typedef struct {
  type_tag tag;
  uint8_t arity;
  union {
    struct {
      NODE_IND_T subs_start;
      NODE_IND_T sub_amt;
    };
    struct {
      NODE_IND_T sub_a;
      NODE_IND_T sub_b;
    };
  };
} type;

#define T_FN_PARAM_IND(node) node.sub_a
#define T_FN_RET_IND(node) node.sub_b
#define T_LIST_SUB_IND(node) node.sub_a
#define T_TUP_SUB_AMT(node) node.sub_amt
#define T_TUP_SUB_IND(inds, node, i) inds[node.subs_start + i]

tree_node_repr type_repr(type_tag tag);
bool inline_types_eq(type a, type b);
void print_type(FILE *f, type *types, NODE_IND_T *inds, NODE_IND_T root);
void print_type_head_placeholders(FILE *f, type_tag head, uint32_t arity);

VEC_DECL(type);
