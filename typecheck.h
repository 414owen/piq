#pragma once

#include "parse_tree.h"
#include "vec.h"
#include "types.h"

typedef struct {
  enum {
    TYPE_NOT_FOUND,
    BINDING_NOT_FOUND,
    TYPE_MISMATCH,
    WRONG_ARITY,
  } err_type;

  NODE_IND_T pos;

  union {
    struct {
      type expected;
      type got;
    };
    struct {
      NODE_IND_T exp_param_amt;
      NODE_IND_T got_param_amt;
    };
  };

} tc_error;

VEC_DECL(tc_error);

typedef struct {
  bool successful;
  vec_tc_error errors;
  // all the types
  vec_type types;
  // type sub-indices
  vec_node_ind type_inds;
  // index into types, one per parse_node
  vec_node_ind node_types;
} tc_res;