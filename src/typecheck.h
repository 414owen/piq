#pragma once

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

typedef struct {
  // all the types
  type *types;
  // type sub-indices
  node_ind_t *type_inds;
  // index into types, one per parse_node
  node_ind_t *node_types;
  node_ind_t type_amt;
} type_info;

// TODO type_inds should be a pointer, not a vec...
typedef struct {
  tc_error *errors;
  node_ind_t error_amt;
  type_info types;
} tc_res;

void print_tc_errors(FILE *, const char *input, parse_tree, tc_res);
tc_res typecheck(const char *input, parse_tree tree);
void free_tc_res(tc_res res);
