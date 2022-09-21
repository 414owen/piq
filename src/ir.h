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

// sizeof: 32
typedef struct {
  span s;
  // TODO remove the binding, when it's part of a ir_fun_group?
  binding b;
  node_ind_t param_pattern_ind;
  node_ind_t body_stmts_start;
  node_ind_t body_stmts_amt;
  node_ind_t type_ind;
} ir_fun;

// sizeof: 24
typedef struct {
  span s;
  node_ind_t param_pattern_ind;
  node_ind_t body_stmts_start;
  node_ind_t body_stmts_amt;
  node_ind_t type_ind;
} ir_fn;

// sizeof: 20
typedef struct {
  span s;
  node_ind_t expr_a_ind;
  node_ind_t expr_b_ind;
  node_ind_t type_ind;
} ir_tup;

// sizeof: 20
typedef struct {
  node_ind_t function_decls_start;
  node_ind_t function_decl_amt;
  node_ind_t data_decls_start;
  node_ind_t data_decl_amt;
  node_ind_t type_ind;
} ir_root;

// sizeof: 20
typedef struct {
  span s;
  node_ind_t callee_expr_ind;
  node_ind_t param_expr_ind;
  node_ind_t type_ind;
} ir_call;

// sizeof: 24
typedef struct {
  span s;
  node_ind_t callee_data_decl_ind;
  node_ind_t callee_constructor_ind;
  node_ind_t param_expr_ind;
  node_ind_t type_ind;
} ir_construction;

// sizeof: 20
typedef struct {
  span s;
  node_ind_t param_ind;
  node_ind_t return_ind;
  node_ind_t type_ind;
} ir_fn_type;

// sizeof: 24
typedef struct {
  span s;
  node_ind_t cond_expr_ind;
  node_ind_t then_expr_ind;
  node_ind_t else_expr_ind;
  node_ind_t type_ind;
} ir_if;

// sizeof: 8
typedef struct {
  span s;
} ir_int;

// sizeof: 16
typedef struct {
  span s;
  node_ind_t exprs_start;
  node_ind_t expr_amt;
  node_ind_t type_ind;
} ir_list;

// sizeof: 12
typedef struct {
  span s;
  node_ind_t inner_type_ind;
  node_ind_t type_ind;
} ir_list_type;

// sizeof: 8
typedef struct {
  span s;
} ir_string;

// sizeof: 16
typedef struct {
  span s;
  node_ind_t expr_ind;
  node_ind_t type_ind;
} ir_as;

// sizeof: 4
typedef struct {
  // always two chars
  buf_ind_t span_start;
} ir_unit;

typedef struct {
  span s;
} ir_binding;

// sizeof: 12
typedef struct {
  span s;
  node_ind_t type_ind;
} ir_upper_name;

// sizeof: 20
typedef struct {
  span s;
  binding binding;
  node_ind_t type_ind;
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

// sizeof: 28
typedef struct {
  span s;
  binding binding;
  ir_expr_type expr_type;
  node_ind_t expr_ind;
  node_ind_t type_ind;
} ir_let;

VEC_DECL(ir_expr_type);

// sizeof: 52
typedef struct {
  ir_sig sig;
  ir_fun fun;
} ir_fun_group;

VEC_DECL(ir_fun_group);

// sizeof: 48
typedef struct {
  ir_sig sig;
  ir_let let;
} ir_let_group;

VEC_DECL(ir_let_group);

// sizeof: 152
typedef struct {
  ir_root ir_root;

  ir_fun_group *ir_fun_groups;
  ir_let_group *ir_let_groups;
  ir_call *ir_calls;
  ir_construction *ir_constructions;
  ir_fn *ir_fns;
  ir_fn_type *ir_fn_types;
  ir_if *ir_ifs;
  ir_int *ir_ints;
  ir_list *ir_lists;
  ir_list_type *ir_list_types;
  ir_string *ir_strings;
  ir_tup *ir_tups;
  ir_as *ir_ass;
  ir_unit *ir_units;
  ir_binding *ir_bindings;

  type *ir_types;
  node_ind_t *ir_node_inds;
} ir_module;

ir_module ir_module_new(parse_tree tree);
void ir_module_free(ir_module *module);
