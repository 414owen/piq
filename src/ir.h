#pragma once

#include "binding.h"
#include "bitset.h"
#include "vec.h"

typedef struct {
  binding type_constructor_name;
  node_ind_t data_constructors_start;
} data_decl;

VEC_DECL(data_decl);

typedef struct {
  binding name;
  node_ind_t fields_start;
  node_ind_t field_amt;
} data_constructor;

VEC_DECL(data_constructor);

typedef struct {
  enum { TYPE_PARAM, TYPE } tag;
  union {
    node_ind_t type_param_ind;
    node_ind_t type_module_ind;
  };
} type_ref;

VEC_DECL(type_ref);

typedef enum {
  R_FUN,
  R_LET,
} bnd_type;

VEC_DECL(bnd_type);

typedef struct {
  vec_data_decl data_decls;
  vec_data_constructor data_constructors;
  vec_type_ref type_refs;
  vec_str_ref refs;
  vec_bnd_type bnd_types;
  bitset ref_is_builtin;
} scope;
