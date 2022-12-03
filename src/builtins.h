#pragma once

#include "types.h"

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>

typedef enum {
  true_builtin = 0,
  false_builtin = 1,

  i8_eq_builtin = 2,
  i16_eq_builtin = 3,
  i32_eq_builtin = 4,
  i64_eq_builtin = 5,

  i8_gt_builtin = 6,
  i16_gt_builtin = 7,
  i32_gt_builtin = 8,
  i64_gt_builtin = 9,

  i8_gte_builtin = 10,
  i16_gte_builtin = 11,
  i32_gte_builtin = 12,
  i64_gte_builtin = 13,

  i8_lt_builtin = 14,
  i16_lt_builtin = 15,
  i32_lt_builtin = 16,
  i64_lt_builtin = 17,

  i8_lte_builtin = 18,
  i16_lte_builtin = 19,
  i32_lte_builtin = 20,
  i64_lte_builtin = 21,
} builtin_term;

extern const char *builtin_type_names[];
extern const type builtin_types[];
extern const node_ind_t named_builtin_type_amount;
extern const node_ind_t builtin_type_amount;
extern const node_ind_t builtin_type_inds[];
extern const node_ind_t builtin_type_ind_amount;
extern const char *builtin_term_names[];
extern const node_ind_t builtin_term_amount;
extern const node_ind_t builtin_term_type_inds[];
