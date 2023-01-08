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

  i8_add_builtin = 22,
  i16_add_builtin = 23,
  i32_add_builtin = 24,
  i64_add_builtin = 25,

  i8_sub_builtin = 26,
  i16_sub_builtin = 27,
  i32_sub_builtin = 28,
  i64_sub_builtin = 29,

  i8_mul_builtin = 30,
  i16_mul_builtin = 31,
  i32_mul_builtin = 32,
  i64_mul_builtin = 33,

  i8_div_builtin = 34,
  i16_div_builtin = 35,
  i32_div_builtin = 36,
  i64_div_builtin = 37,

  i8_rem_builtin = 38,
  i16_rem_builtin = 39,
  i32_rem_builtin = 40,
  i64_rem_builtin = 41,

  // mod_floor
  i8_mod_builtin = 42,
  i16_mod_builtin = 43,
  i32_mod_builtin = 44,
  i64_mod_builtin = 45,

  builtin_term_amount = 46,
} builtin_term;

enum {
  bool_type_ind = 0,
  u8_type_ind = 1,
  u16_type_ind = 2,
  u32_type_ind = 3,
  u64_type_ind = 4,
  i8_type_ind = 5,
  i16_type_ind = 6,
  i32_type_ind = 7,
  i64_type_ind = 8,
  string_type_ind = 9,

  compare_i8s_type_ind = 10,
  compare_i16s_type_ind = 11,
  compare_i32s_type_ind = 12,
  compare_i64s_type_ind = 13,

  i8_arithmetic_type_ind = 14,
  i16_arithmetic_type_ind = 15,
  i32_arithmetic_type_ind = 16,
  i64_arithmetic_type_ind = 17,
};

extern const char *builtin_type_names[];
extern const type builtin_types[];
extern const node_ind_t named_builtin_type_amount;
extern const node_ind_t builtin_type_amount;
extern const node_ind_t builtin_type_inds[];
extern const node_ind_t builtin_type_ind_amount;
extern const char *builtin_term_names[];
  [string_type_ind] = "String",
};

enum {
  i8_arithmetic_fn_ind_start = builtin_term_amount + 0,
  i16_arithmetic_fn_ind_start = builtin_term_amount + 3,
  i32_arithmetic_fn_ind_start = builtin_term_amount + 6,
  i64_arithmetic_fn_ind_start = builtin_term_amount + 9,

  u8_arithmetic_fn_ind_start = builtin_term_amount + 12,
  u16_arithmetic_fn_ind_start = builtin_term_amount + 15,
  u32_arithmetic_fn_ind_start = builtin_term_amount + 18,
  u64_arithmetic_fn_ind_start = builtin_term_amount + 21,

  i8_predicate_fn_ind_start = builtin_term_amount + 24,
  i16_predicate_fn_ind_start = builtin_term_amount + 27,
  i32_predicate_fn_ind_start = builtin_term_amount + 30,
  i64_predicate_fn_ind_start = builtin_term_amount + 33,

  u8_predicate_fn_ind_start = builtin_term_amount + 36,
  u16_predicate_fn_ind_start = builtin_term_amount + 39,
  u32_predicate_fn_ind_start = builtin_term_amount + 42,
  u64_predicate_fn_ind_start = builtin_term_amount + 45,
