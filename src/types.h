// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "ast_meta.h"
#include "consts.h"
#include "hashmap.h"
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
    // includes type variables, used in typecheck phase
    type_check_tag check;
    // no type variables
    type_tag normal;
  } tag;

  union {
    typevar type_var;
    struct {
      type_ref start;
      type_ref amt;
    } more_subs;
    struct {
      type_ref ind;
    } one_sub;
    struct {
      type_ref a;
      type_ref b;
    } two_subs;
  } data;
} type;

VEC_DECL(type);

typedef struct {
  vec_type types;
  a_hashmap type_to_index;
  // The indices here are going to have a lot of overlap.
  // For example, the type builder
  // .types = {
  //   {
  //     .tag = T_FN,
  //     .subs_start = 0,
  //     .sub_amt = 4,
  //   },
  //   {
  //     .tag = T_FN,
  //     .subs_s/tart = 0,
  //     .sub_amt = 3,
  //   },
  //   { .tag = T_I32, },
  //   { .tag = T_I16, },
  //   { .tag = T_I8,  },
  //   { .tag = T_U8,  },
  // }
  // .inds = {
  //   2, 3, 4, 5
  // }
  //

  // Reuses indices 0-3 in two function types, and is perfectly valid.
  // Unfortunately, we don't currently reuse runs of indices, unless we're
  // reuing the whole type. I had a quick go at reusing runs using suffix
  // arrays, but in the end I got fed up and deleted the work. See commit
  // c23046ca2496f19ea165aaf9c078397db597d410 if you want to revive the work.
  vec_type_ref inds;

  // Looking back at these, I think they're used in different parts of the
  // typechecker. I'll comment if I remember.
  union {
    vec_type_ref node_types;
    vec_type_ref substitutions;
  } data;
} type_builder;

typedef struct {
  type *nodes;
  type_ref *inds;
} type_tree;

#define T_FN_PARAM_AMT(node) ((node).data.more_subs.amt - 1)
#define T_FN_PARAM_IND(inds, node, i) (inds)[(node).data.more_subs.start + i]
#define T_FN_RET_IND(inds, node) (inds)[(node).data.more_subs.start + (node).data.more_subs.amt - 1]

#define T_OR_SUB_AMT(node) ((node).data.more_subs.amt)
#define T_OR_SUB_IND(inds, node, i) (inds)[(node).data.more_subs.start + i]

#define T_LIST_SUB_IND(node) node.data.one_sub.ind

#define T_TUP_SUB_A(node) node.data.two_subs.a
#define T_TUP_SUB_B(node) node.data.two_subs.b

void free_type_builder(type_builder tb);
tree_node_repr type_repr(type_check_tag tag);
bool inline_types_eq(type a, type b);
void print_type(FILE *f, type *types, node_ind_t *inds, node_ind_t root);
char *print_type_str(type *types, node_ind_t *inds, node_ind_t root);
void print_type_head_placeholders(FILE *f, type_check_tag head);

node_ind_t mk_primitive_type(type_builder *tb, type_check_tag tag);
node_ind_t mk_type_inline(type_builder *tb, type_check_tag tag,
                          node_ind_t sub_a, node_ind_t sub_b);
node_ind_t mk_type(type_builder *tb, type_check_tag tag, const node_ind_t *subs,
                   node_ind_t sub_amt);
node_ind_t mk_type_var(type_builder *tb, typevar value);
void push_type_subs(vec_type_ref *restrict stack, const type_ref *restrict inds,
                    type t);

type_builder new_type_builder(void);
type_builder new_type_builder_with_builtins(void);

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

// Used as a key in hashmaps, where we haven't yet added the type
// to the builder.
typedef struct {
  type_check_tag tag;
  union {
    struct {
      type_ref sub_amt;
      const type_ref *subs;
    };
    struct {
      type_ref sub_a;
      type_ref sub_b;
    };
  };
} type_key_with_ctx;

bool type_contains_specific_typevar(const type_builder *types, type_ref root,
                                    typevar a);
bool type_contains_unsubstituted_typevar(const type_builder *builder,
                                         type_ref root,
                                         node_ind_t parse_node_amount);
