// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include "defs.h"
#include "parse_tree.h"
#include "vec.h"
#include "types.h"

typedef enum {
  // INT_SMALLER_THAN_MIN,
  // INT_LARGER_THAN_MAX,
  // TYPE_NOT_FOUND,
  // AMBIGUOUS_TYPE,
  // BINDING_NOT_FOUND,
  // WRONG_ARITY,
  // NEED_SIGNATURE,
  // CALLED_NON_FUNCTION,

  // TODO Do we need this?
  // TYPE_MISMATCH,

  // TYPE_HEAD_MISMATCH,
  // TUPLE_WRONG_ARITY,
  // LITERAL_MISMATCH,
  // MISSING_SIG,
  // BLOCK_ENDS_IN_STATEMENT,

  TC_ERR_CONFLICT,
  TC_ERR_AMBIGUOUS,
  TC_ERR_INFINITE,
} tc_error_type;

typedef struct {
  tc_error_type type;
  node_ind_t pos;

  union {
    struct {
      type_ref expected_ind;
      type_ref got_ind;
    } conflict;
    struct {
      type_ref index;
    } infinite;
    struct {
      type_ref index;
    } ambiguous;
  };
} tc_error;

VEC_DECL(tc_error);

typedef struct {
  type_tree tree;
  // index into types, one per parse_node
  type_ref *node_types;
  type_ref type_amt;
} type_info;

typedef struct {
  tc_error *errors;
  node_ind_t error_amt;
  type_info types;
#ifdef TIME_TYPECHECK
  perf_values perf_values;
#endif
} tc_res;

void print_tc_errors(FILE *, const char *input, parse_tree, tc_res);
tc_res typecheck(parse_tree tree);
void free_tc_res(tc_res res);
