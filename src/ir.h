// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include "binding.h"
#include "bitset.h"
#include "parse_tree.h"
#include "slice.h"
#include "span.h"
#include "types.h"
#include "vec.h"

#define IR_MODULE_ALLOC_PTR ir_fun_groups

/*
  Struct of arrays, name-resolved parse_tree representation.
  This is a memory-efficient representation, but it's not
  optimised for access patterns.

  Ideally, what we want to do is tell the compiler "I have a tagged union,
  please store it as a struct of arrays, where all same-size variants are
  in the same array". I guess I'll implement that in the language, but
  it's just too much effort to do manually in C, so here we're storing
  an array of each variant type.
*/

typedef struct {
  node_ind_t pt_root;
  node_ind_t pt_call;
  node_ind_t pt_construction;
  node_ind_t pt_fn;
  node_ind_t pt_fn_type;
  // stored inline
  // node_ind_t pt_fun_body;
  node_ind_t pt_if;
  node_ind_t pt_int;
  node_ind_t pt_list;
  node_ind_t pt_list_type;
  node_ind_t pt_string;
  node_ind_t pt_tup;
  node_ind_t pt_as;
  node_ind_t pt_unit;
  node_ind_t pt_fun;
  node_ind_t pt_sig;
  node_ind_t pt_let;
  node_ind_t pt_binding;
} pt_node_amounts;

typedef struct {
  node_ind_t n;
} ir_expression_ind;

typedef struct {
  node_ind_t n;
} ir_fn_ind;

typedef struct {
  node_ind_t n;
} ir_type_ind;

typedef struct {
  node_ind_t n;
} ir_function_decl_ind;

typedef struct {
  node_ind_t n;
} ir_data_decl_ind;

typedef struct {
  node_ind_t n;
} data_constructor_ind;

typedef struct {
  node_ind_t n;
} ir_pattern_ind;

typedef struct {
  ir_data_decl_ind ir_data_constructor_ref_data_decl_ind;
  data_constructor_ind ir_data_constructor_ref_constructor_ind;
} ir_data_constructor_ref;

typedef enum {
  IR_PAT_UNIT,
  IR_PAT_PLACEHOLDER,
  IR_PAT_LIST,
  IR_PAT_TUP,
  IR_PAT_CONSTRUCTION,
  IR_PAT_INT,
  IR_PAT_STRING,
} ir_pattern_type;

typedef struct {
  ir_pattern_type ir_pattern_tag;
  ir_type_ind ir_pattern_type;

  union {
    struct {
      ir_pattern_ind ir_pattern_tup_left_ind;
      ir_pattern_ind ir_pattern_tup_right_ind;
    };
    struct {
      ir_data_constructor_ref ir_pattern_data_construction_callee_ind;
      ir_pattern_ind ir_pattern_data_construction_param_ind;
    };
    node_slice subs;
    span ir_pattern_binding_span;
    span ir_pattern_int_span;
    span ir_pattern_str_span;
  };
} ir_pattern;

VEC_DECL(ir_pattern);

typedef struct {
  ir_pattern ir_fn_param;
  node_ind_t ir_fn_body_statements_start;
  node_ind_t ir_fn_body_statements_amt;
  node_ind_t ir_fn_type_ind;
} ir_fn;

VEC_DECL(ir_fn);

typedef struct {
  ir_pattern ir_fun_param;
  node_ind_t ir_fun_body_statements_start;
  node_ind_t ir_fun_body_statements_amt;
} ir_fun_case;

VEC_DECL(ir_fun_case);

// Yes, this is needed, you can't reconstruct this from the *type* information
typedef enum {
  IR_EXPRESSION_TUP,
  IR_EXPRESSION_CALL,
  IR_EXPRESSION_FN,
  IR_EXPRESSION_IF,
  IR_EXPRESSION_INT,
  IR_EXPRESSION_CONSTRUCTOR,
} ir_expression_type;

typedef struct {
  ir_expression_type ir_expression_tag;
  ir_type_ind ir_expression_type;
  union {
    uint8_t ir_expression_u8;
    int8_t ir_expression_i8;
    uint16_t ir_expression_u16;
    int16_t ir_expression_i16;
    uint32_t ir_expression_u32;
    int32_t ir_expression_i32;
    // tuples, strings, lists, {i,u}64, calls
    ir_expression_ind ir_expression_ind;
  };
} ir_expression;

VEC_DECL(ir_expression);

typedef struct {
  ir_type_ind ir_tup_type_ind;
  ir_expression ir_tup_expression_a;
  ir_expression ir_tup_expression_b;
} ir_tup;

VEC_DECL(ir_tup);

// Why doesn't this contain everything?
// Well, I think we'll want to generate functions, so they wouldn't belong here.
typedef struct {
  ir_function_decl_ind ir_root_function_decls_start;
  ir_function_decl_ind ir_root_function_decl_amt;
  ir_data_decl_ind ir_root_data_decls_start;
  ir_data_decl_ind ir_root_data_decl_amt;
} ir_root;

typedef struct {
  // don't need type, it's in the expression
  // ir_type_ind ir_call_type_ind;
  ir_expression ir_call_callee;
  ir_expression ir_call_param;
} ir_call;

VEC_DECL(ir_call);

typedef struct {
  // not needed, provided by ir_expression
  // ir_type_ind ir_data_construction_type_ind;
  ir_data_constructor_ref ir_data_construction_callee_ref;
  ir_expression ir_data_construction_param;
} ir_data_construction;

VEC_DECL(ir_data_construction);

typedef struct {
  ir_type_ind ir_fun_type_ind;
  ir_type_ind ir_fun_param_ind;
  ir_type_ind ir_fun_return_ind;
} ir_fn_type;

VEC_DECL(ir_fn_type);

typedef struct {
  ir_type_ind ir_if_type_ind;
  ir_expression ir_if_cond_expression;
  ir_expression ir_if_then_expression;
  ir_expression ir_if_else_expression;
} ir_if;

VEC_DECL(ir_if);

typedef struct {
  ir_type_ind ir_list_type_ind;
  ir_expression_ind ir_list_exressions_start;
  ir_expression_ind ir_list_expression_amt;
} ir_list;

VEC_DECL(ir_list);

// TODO delete?
typedef struct {
  ir_type_ind ir_list_type_inner_type;
} ir_list_type;

VEC_DECL(ir_list_type);

// TODO: inline into expression?
typedef span ir_string;

VEC_DECL(ir_string);

typedef struct {
  ir_type_ind ir_as_type_ind;
  ir_expression ir_as_expression_ind;
} ir_as;

VEC_DECL(ir_as);

typedef struct {
  span s;
} ir_binding;

typedef struct {
  // TODO
  ir_expression_ind ir_upper_name;
} ir_upper_name;

typedef struct {
  binding ir_let_binding;
  ir_expression ir_let_expression;
  ir_type_ind ir_let_type_ind;
} ir_let;

VEC_DECL(ir_expression_type);

typedef struct {
  binding b;
  ir_type_ind ir_sig;
  ir_fn_ind ir_fun_group_clauses_start;
  ir_fn_ind ir_fun_group_clause_amt;
} ir_fun_group;

VEC_DECL(ir_fun_group);

typedef struct {
  ir_type_ind ir_sig;
  ir_let let;
} ir_let_group;

VEC_DECL(ir_let_group);

// If you want to add something here, first add it to test_ir.c
// then run `test -m 'IR/layout'` to find out where it belongs.

typedef struct {
  // sizeof el: 16
  ir_root ir_root;

  // sizeof: 40
  vec_ir_if ir_ifs;

  // sizeof: 28
  union {
    vec_ir_let_group ir_let_groups;
    vec_ir_fn ir_fns;
    vec_ir_tup ir_tups;
  };

  // sizeof: 24
  union {
    vec_ir_call ir_calls;
    vec_ir_data_construction ir_data_constructions;
  };

  vec_ir_fun_case ir_funs;

  // sizeof: 20
  vec_ir_fun_group ir_fun_groups;

  // sizeof: 16
  union {
    vec_ir_as ir_ass;
    vec_ir_pattern ir_patterns;
  };

  // sizeof: 12
  union {
    vec_ir_fn_type ir_fn_types;
    vec_ir_list ir_lists;
  };

  // sizeof: 8
  vec_ir_string ir_strings;

  // sizeof: 4
  vec_ir_list_type ir_list_types;

  vec_type types;
  vec_node_ind node_inds;
} ir_module;
