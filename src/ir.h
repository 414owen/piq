#pragma once

#include "binding.h"
#include "bitset.h"
#include "vec.h"

typedef struct {
  binding type_constructor_name;
  node_ind_t data_constructors_start;
} ir_data_decl;

VEC_DECL(ir_data_decl);

typedef struct {
  binding name;
  node_ind_t fields_start;
  node_ind_t field_amt;
} ir_data_constructor;

VEC_DECL(ir_data_constructor);

typedef struct {
  enum { TYPE_PARAM, TYPE } tag;
  union {
    node_ind_t type_param_ind;
    node_ind_t type_module_ind;
  };
} ir_type_ref;

VEC_DECL(ir_type_ref);

typedef enum {
  R_FUN,
  R_LET,
} bnd_type;

VEC_DECL(bnd_type);

typedef struct {
  vec_ir_data_decl ir_data_decls;
  vec_ir_data_constructor ir_data_constructors;
  vec_ir_type_ref ir_type_refs;
  vec_str_ref ir_refs;
  vec_bnd_type ir_bnd_types;
  bitset ref_is_builtin;
} ir_module;
