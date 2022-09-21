#pragma once

#include "ir.h"
#include "parse_tree.h"
#include "vec.h"
#include "types.h"

typedef enum {
  INT_LARGER_THAN_MAX,
  TYPE_NOT_FOUND,
  AMBIGUOUS_TYPE,
  BINDING_NOT_FOUND,
  WRONG_ARITY,
  NEED_SIGNATURE,
  CALLED_NON_FUNCTION,

  // TODO Do we need this?
  TYPE_MISMATCH,

  TYPE_HEAD_MISMATCH,
  TUPLE_WRONG_ARITY,
  LITERAL_MISMATCH,
  MISSING_SIG,
} tc_error_type;

typedef struct {
  tc_error_type type;
  node_ind_t pos;

  union {
    struct {
      node_ind_t expected;
      union {
        node_ind_t got;
        type_tag got_type_head;
      };
    };
    struct {
      node_ind_t exp_param_amt;
      node_ind_t got_param_amt;
    };
  };
} tc_error;

VEC_DECL(tc_error);

// TODO type_inds should be a pointer, not a vec...
typedef struct {
  ir_module module;
  vec_tc_error errors;
  // all the types
  vec_type types;
  // type sub-indices
  vec_node_ind type_inds;
  // index into types, one per parse_node
  node_ind_t *node_types;
} tc_res;

void print_tc_errors(FILE *, source_file, parse_tree, tc_res);
tc_res typecheck(source_file source, parse_tree tree);
void free_tc_res(tc_res res);
