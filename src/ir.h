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

  Ideally, whate we want to do is tell the compiler "I have a tagged union,
  please store it as a struct of arrays, where all same-size variants are
  in the same array". I guess I'll implement that in the language, but
  it's just too much effort to do manually in C, so here we're storing
  an array of each variant type.
*/

// sizeof: 28
typedef struct {
  span s;
  // TODO remove the binding, when it's part of a ir_fun_group?
  binding b;
  node_ind_t param_pattern_ind;
  node_ind_t body_start;
  node_ind_t body_amt;
} ir_fun;

// sizeof: 20
typedef struct {
  span s;
  node_ind_t param_pattern_ind;
  node_ind_t body_start;
  node_ind_t body_amt;
} ir_fn;

// sizeof: 16
typedef struct {
  span s;
  node_ind_t expr_a_ind;
  node_ind_t expr_b_ind;
} ir_tup;

// sizeof: 16
typedef struct {
  node_ind_t function_decls_start;
  node_ind_t function_decl_amt;
  node_ind_t data_decls_start;
  node_ind_t data_decl_amt;
} ir_root;

// sizeof: 16
typedef struct {
  span s;
  node_ind_t callee_expr_ind;
  node_ind_t param_expr_ind;
} ir_call;

// sizeof: 20
typedef struct {
  span s;
  node_ind_t callee_data_decl_ind;
  node_ind_t callee_constructor_ind;
  node_ind_t param_expr_ind;
} ir_construction;

// sizeof: 16
typedef struct {
  span s;
  node_ind_t param_ind;
  node_ind_t return_ind;
} ir_fn_type;

// sizeof: 16
typedef struct {
  span s;
  node_ind_t stmts_start;
  node_ind_t stmt_amt;
} ir_fun_body;

// sizeof: 20
typedef struct {
  span s;
  node_ind_t cond_expr_ind;
  node_ind_t then_expr_ind;
  node_ind_t else_expr_ind;
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
} ir_list;

// sizeof: 12
typedef struct {
  span s;
  node_ind_t inner_type_ind;
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

// sizeof: 8
typedef struct {
  span s;
} ir_unit;

// sizeof: 8
typedef struct {
  span s;
} ir_upper_name;

// sizeof: 20
typedef struct {
  span s;
  binding binding;
  node_ind_t type_ind;
} ir_sig;

// sizeof: 20
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

// sizeof: 48
typedef struct {
  ir_sig sig;
  ir_fun fun;
} ir_fun_group;

VEC_DECL(ir_fun_group);

// sizeof: 40
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
