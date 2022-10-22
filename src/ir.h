#pragma once

#include "binding.h"
#include "bitset.h"
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
} ir_expr_ind;

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

typedef struct {
  enum {
    PAT_TUP,
    PAT_CONSTRUCT,
    PAT_INT,
    PAT_STR,
    PAT_UNIT,
  } tag;

  union {
    struct {
      ir_pattern_ind ir_pattern_tup_left_ind;
      ir_pattern_ind ir_pattern_tup_right_ind;
    };
    struct {
      ir_data_constructor_ref ir_pattern_data_construction_callee_ind;
      ir_pattern_ind ir_pattern_data_construction_param_ind;
    };
    span ir_pattern_int_span;
    span ir_pattern_str_span;
  };
} ir_pattern;

typedef struct {
  ir_pattern ir_fn_param;
  node_ind_t ir_fn_body_stmts_start;
  node_ind_t ir_fn_body_stmts_amt;
  node_ind_t ir_fn_type_ind;
} ir_fn;

VEC_DECL(ir_fn);

typedef struct {
  ir_pattern ir_fn_param;
  node_ind_t ir_fn_body_stmts_start;
  node_ind_t ir_fn_body_stmts_amt;
  node_ind_t ir_fn_type_ind;
} ir_fun;

VEC_DECL(ir_fun);

typedef enum {
  EXPR_TUP,
  EXPR_STR,
  EXPR_LIST,
  EXPR_U8,
  EXPR_I8,
  EXPR_U16,
  EXPR_I16,
  EXPR_U32,
  EXPR_I32,
  EXPR_U64,
  EXPR_I64,
} expr_type;

typedef struct {
  ir_type_ind type;
  expr_type ir_expr_tag;

  union {
    uint8_t ir_expr_u8;
    int8_t ir_expr_i8;
    uint16_t ir_expr_u16;
    int16_t ir_expr_i16;
    uint32_t ir_expr_u32;
    int32_t ir_expr_i32;
    // tuples, strings, lists, {i,u}64, calls
    ir_expr_ind ir_expr_ind;
  };
} ir_expr;

typedef struct {
  ir_type_ind ir_tup_type_ind;
  ir_expr ir_tup_expr_a;
  ir_expr ir_tup_expr_b;
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
  ir_type_ind ir_call_type_ind;
  ir_expr ir_call_expr_a;
  ir_expr ir_call_expr_b;
} ir_call;

VEC_DECL(ir_call);

typedef struct {
  ir_type_ind ir_data_construction_type_ind;
  ir_data_constructor_ref ir_data_construction_callee_ref;
  ir_expr ir_data_construction_param;
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
  ir_expr ir_if_cond_expr;
  ir_expr ir_if_then_expr;
  ir_expr ir_if_else_expr;
} ir_if;

VEC_DECL(ir_if);

typedef struct {
  ir_type_ind ir_list_type_ind;
  ir_expr_ind ir_list_exprs_start;
  ir_expr_ind ir_list_expr_amt;
} ir_list;

VEC_DECL(ir_list);

// TODO delete?
typedef struct {
  ir_type_ind ir_list_type_inner_type;
} ir_list_type;

VEC_DECL(ir_list_type);

// TODO: inline into expr?
typedef struct {
  span s;
} ir_string;

VEC_DECL(ir_string);

typedef struct {
  ir_type_ind ir_as_type_ind;
  ir_expr ir_as_expr_ind;
} ir_as;

VEC_DECL(ir_as);

typedef struct {
  span s;
} ir_binding;

typedef struct {
  // TODO
  ir_expr_ind ir_upper_name;
} ir_upper_name;

typedef struct {
} ir_sig;

typedef enum {
  IR_EXPR_TUP,
  IR_EXPR_CALL,
  IR_EXPR_FN,
  IR_EXPR_IF,
  IR_EXPR_INT,
  IR_EXPR_EXPR_REF,
  IR_EXPR_CONSTRUCTOR_REF,
} ir_expr_type;

typedef struct {
  binding ir_let_binding;
  ir_expr_type ir_let_expr_type;
  ir_expr ir_let_expr;
  ir_type_ind ir_let_type_ind;
} ir_let;

VEC_DECL(ir_expr_type);

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

/*
typedef struct {
  ir_root ir_root;

  ir_fun_group *ir_fun_groups;
  ir_let_group *ir_let_groups;
  ir_call *ir_calls;
  ir_data_construction *ir_constructions;
  ir_fn *ir_fns;
  ir_fn_type *ir_fn_types;
  ir_if *ir_ifs;
  ir_list *ir_lists;
  ir_list_type *ir_list_types;
  ir_string *ir_strings;
  ir_tup *ir_tups;
  ir_as *ir_ass;

  type *ir_types;
  node_ind_t *ir_node_inds;
} ir_module;
*/

// TODO:
// for every set of equally-sized node types,
// union the vectors, for a single compact heterogeneous-element
typedef struct {
  // sizeof el: 16
  ir_root ir_root;

  // sizeof: 40
  vec_ir_if ir_ifs;

  // sizeof: 32
  vec_ir_let_group ir_let_groups;

  // sizeof: 28
  union {
    vec_ir_call ir_calls;
    vec_ir_fn ir_fns;
    vec_ir_tup ir_tups;
  };

  // sizeof: 24
  vec_ir_data_construction ir_data_constructions;

  // sizeof: 20
  vec_ir_fun_group ir_fun_groups;

  // sizeof: 16
  vec_ir_as ir_ass;

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
