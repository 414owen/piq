#pragma once

#include "binding.h"
#include "bitset.h"
#include "span.h"
#include "vec.h"

typedef struct {
  span s;
  binding type_constructor_name;
  node_ind_t data_constructors_start;
  node_ind_t data_constructor_amt;
} data_decl;

VEC_DECL(data_decl);

typedef struct {
  span s;
  binding name;
  node_ind_t fields_start;
  node_ind_t field_amt;
} data_constructor;

VEC_DECL(data_constructor);

// inline?
typedef struct {
  enum { TYPE_PARAM, TYPE } tag;
  union {
    node_ind_t type_param_ind;
    node_ind_t type_ind;
  };
} type_ref;

typedef struct {
  span s;
  binding binding;
  type_ref type;
} sig;

typedef struct {
  node_ind_t sig;
  binding bnd;
  node_ind_t cases_start;
  node_ind_t case_amt;
} fun_group_decl;

VEC_DECL(fun_group_decl);

typedef struct {
  span s;
  node_ind_t pattern;
  node_ind_t body_start;
  node_ind_t body_amt;
} fun_decl;

VEC_DECL(fun_decl);

typedef enum {
  R_FUN,
  R_LET,
} bnd_type;

VEC_DECL(bnd_type);

typedef struct {
  vec_data_decl data_decls;
  vec_data_constructor data_constructors;
  vec_fun_group_decl funs;
  vec_str_ref refs;
  vec_bnd_type bnd_types;
  bitset ref_is_builtin;
} scope;
