#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "ast_meta.h"
#include "consts.h"
#include "vec.h"

typedef enum __attribute__((packed)) {
  T_UNKNOWN,
  T_UNIT,
  T_I8,
  T_U8,
  T_I16,
  T_U16,
  T_I32,
  T_U32,
  T_I64,
  T_U64,
  T_FN,
  T_BOOL,
  T_TUP,
  T_LIST,
  T_CALL
} type_tag;

typedef struct {
  type_tag tag;
  union {
    struct {
      node_ind_t subs_start;
      node_ind_t sub_amt;
    };
    struct {
      node_ind_t sub_a;
      node_ind_t sub_b;
    };
  };
} type;

#define T_FN_PARAM_AMT(node) ((node).sub_amt - 1)
#define T_FN_PARAM_IND(inds, node, i) inds[(node).subs_start + i]
#define T_FN_RET_IND(inds, node) inds[(node).subs_start + (node).sub_amt - 1]

#define T_LIST_SUB_IND(node) node.sub_a
#define T_TUP_SUB_AMT(node) 2
#define T_TUP_SUB_A(node) node.sub_a
#define T_TUP_SUB_B(node) node.sub_b

tree_node_repr type_repr(type_tag tag);
bool inline_types_eq(type a, type b);
void print_type(FILE *f, type *types, node_ind_t *inds, node_ind_t root);
void print_type_head_placeholders(FILE *f, type_tag head);

VEC_DECL(type);
