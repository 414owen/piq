#pragma once

#include "binding.h"
#include "bitset.h"
#include "span.h"
#include "types.h"
#include "vec.h"

#define IR_MODULE_ALLOC_PTR ir_fun_groups

/*
  Struct of arrays, name-resolved parse_tree representation.
*/

typedef struct {
  span s;
  binding b;
  node_ind_t param_pattern_ind;
  node_ind_t body_start;
  node_ind_t body_amt;
} ir_fun;

typedef struct {
  span s;
  node_ind_t param_pattern_ind;
  node_ind_t body_start;
  node_ind_t body_amt;
} ir_fn;

typedef struct {
  span s;
  node_ind_t expr_a_ind;
  node_ind_t expr_b_ind;
} ir_tup;

typedef struct {
  node_ind_t function_decls_start;
  node_ind_t function_decl_amt;
  node_ind_t data_decls_start;
  node_ind_t data_decl_amt;
} ir_root;

typedef struct {
  span s;
  node_ind_t callee_expr_ind;
  node_ind_t param_expr_ind;
} ir_call;

typedef struct {
  span s;
  node_ind_t callee_data_decl_ind;
  node_ind_t callee_constructor_ind;
  node_ind_t param_expr_ind;
} ir_construction;

typedef struct {
  span s;
  node_ind_t param_ind;
  node_ind_t return_ind;
} ir_fn_type;

typedef struct {
  span s;
  node_ind_t stmts_start;
  node_ind_t stmt_amt;
} ir_fun_body;

typedef struct {
  span s;
  node_ind_t cond_expr_ind;
  node_ind_t then_expr_ind;
  node_ind_t else_expr_ind;
} ir_if;

typedef struct {
  span s;
} ir_int;

typedef struct {
  span s;
  node_ind_t exprs_start;
  node_ind_t expr_amt;
} ir_list;

typedef struct {
  span s;
  node_ind_t inner_type_ind;
} ir_list_type;

typedef struct {
  span s;
} ir_string;

typedef struct {
  span s;
  node_ind_t expr_ind;
  node_ind_t type_ind;
} ir_as;

typedef struct {
  span s;
} ir_unit;

typedef struct {
  span s;
} ir_upper_name;

typedef struct {
  span s;
  binding binding;
  node_ind_t type_ind;
} ir_sig;

typedef struct {
  span s;
  binding binding;
  node_ind_t expr_ind;
} ir_let;

typedef enum {
  IR_EXPR_TUP,
  IR_EXPR_CALL,
  IR_EXPR_FN,
  IR_EXPR_IF,
  IR_EXPR_INT,
  IR_EXPR_EXPR_REF,
  IR_EXPR_CONSTRUCTOR_REF,
} ir_expr_type;

VEC_DECL(ir_expr_type);

typedef struct {
  node_ind_t sig_ind;
  node_ind_t fun_ind;
} ir_fun_group;

VEC_DECL(ir_fun_group);

typedef struct {
  node_ind_t sig_ind;
  node_ind_t let_ind;
  // TODO consider storing inline
} ir_let_group;

VEC_DECL(ir_let_group);

typedef struct {
  ir_root ir_root;

  ir_fun_group *ir_fun_groups;
  ir_let_group *ir_let_groups;
  ir_call *ir_calls;
  ir_construction *ir_constructions;
  ir_fn *ir_fns;
  ir_fn_type *ir_fn_types;
  ir_fun_body *ir_fun_bodies;
  ir_if *ir_ifs;
  ir_int *ir_ints;
  ir_list *ir_lists;
  ir_list_type *ir_list_types;
  ir_string *ir_strings;
  ir_tup *ir_tups;
  ir_as *ir_ass;
  ir_unit *ir_units;

  type *ir_types;

  // Used for types, expr_refs, constructor_refs
  node_ind_t *ir_node_inds;
} ir_module;

void ir_module_free(ir_module *);
