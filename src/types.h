#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "ast_meta.h"
#include "consts.h"
#include "vec.h"

typedef enum {
  TC_UNIT,
  TC_I8,
  TC_U8,
  TC_I16,
  TC_U16,
  TC_I32,
  TC_U32,
  TC_I64,
  TC_U64,
  TC_FN,
  TC_BOOL,
  TC_TUP,
  TC_LIST,
  TC_CALL,
  TC_VAR,
  TC_OR,
} type_check_tag;

typedef enum {
  T_UNIT = TC_UNIT,
  T_I8 = TC_I8,
  T_U8 = TC_U8,
  T_I16 = TC_I16,
  T_U16 = TC_U16,
  T_I32 = TC_I32,
  T_U32 = TC_U32,
  T_I64 = TC_I64,
  T_U64 = TC_U64,
  T_FN = TC_FN,
  T_BOOL = TC_BOOL,
  T_TUP = TC_TUP,
  T_LIST = TC_LIST,
  T_CALL = TC_CALL,
} type_tag;

typedef node_ind_t type_ref;

VEC_DECL(type_ref);

typedef node_ind_t typevar;

VEC_DECL(typevar);

typedef struct {
  union {
    type_tag tag;
    type_check_tag check_tag;
  };
  union {
    typevar type_var;
    struct {
      type_ref subs_start;
      type_ref sub_amt;
    };
    struct {
      type_ref sub_a;
      type_ref sub_b;
    };
  };
} type;

VEC_DECL(type);

typedef struct {
  vec_type types;
  vec_type_ref inds;
  union {
    vec_type_ref node_types;
    vec_type_ref substitutions;
  };
} type_builder;

typedef struct {
  type *nodes;
  type_ref *inds;
} type_tree;

#define T_FN_PARAM_AMT(node) ((node).sub_amt - 1)
#define T_FN_PARAM_IND(inds, node, i) (inds)[(node).subs_start + i]
#define T_FN_RET_IND(inds, node) (inds)[(node).subs_start + (node).sub_amt - 1]

#define T_OR_SUB_AMT(node) ((node).sub_amt)
#define T_OR_SUB_IND(inds, node, i) (inds)[(node).subs_start + i]

#define T_LIST_SUB_IND(node) node.sub_a
#define T_TUP_SUB_AMT(node) 2
#define T_TUP_SUB_A(node) node.sub_a
#define T_TUP_SUB_B(node) node.sub_b

void free_type_builder(type_builder tb);
tree_node_repr type_repr(type_check_tag tag);
bool inline_types_eq(type a, type b);
void print_type(FILE *f, type *types, node_ind_t *inds, node_ind_t root);
char *print_type_str(type *types, node_ind_t *inds, node_ind_t root);
void print_type_head_placeholders(FILE *f, type_check_tag head);

node_ind_t find_primitive_type(type_builder *tb, type_check_tag tag);
node_ind_t find_type(type_builder *tb, type_check_tag tag,
                     const node_ind_t *subs, node_ind_t sub_amt);
node_ind_t mk_primitive_type(type_builder *tb, type_check_tag tag);
node_ind_t mk_type_inline(type_builder *tb, type_check_tag tag,
                          node_ind_t sub_a, node_ind_t sub_b);
node_ind_t mk_type(type_builder *tb, type_check_tag tag, const node_ind_t *subs,
                   node_ind_t sub_amt);
node_ind_t mk_type_var(type_builder *tb, typevar value);
void push_type_subs(vec_type_ref *restrict stack, const type_ref *restrict inds,
                    type t);

/*
typedef struct {
  const type *types;
  const node_ind_t *inds;
  vec_type_ref stack;
} type_traversal;

typedef struct {
  type_ref index;
  type type;
} type_traversal_elem;

type_traversal traverse_types(types);
*/

bool type_contains_specific_typevar(const type_builder *types, type_ref root,
                                    typevar a);
bool type_contains_unsubstituted_typevar(const type_builder *builder,
                                         type_ref root,
                                         node_ind_t parse_node_amount);
