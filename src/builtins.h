#pragma once

#include "types.h"

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>

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

  compare_u8s_type_ind = 14,
  compare_u16s_type_ind = 15,
  compare_u32s_type_ind = 16,
  compare_u64s_type_ind = 17,

  i8_arithmetic_type_ind = 18,
  i16_arithmetic_type_ind = 19,
  i32_arithmetic_type_ind = 20,
  i64_arithmetic_type_ind = 21,

  u8_arithmetic_type_ind = 22,
  u16_arithmetic_type_ind = 23,
  u32_arithmetic_type_ind = 24,
  u64_arithmetic_type_ind = 25,

  any_int_type_ind = 26,
};

typedef enum {
  true_builtin = 0,
  false_builtin = 1,

  i8_eq_builtin = 2,
  i16_eq_builtin = 3,
  i32_eq_builtin = 4,
  i64_eq_builtin = 5,

  u8_eq_builtin = 6,
  u16_eq_builtin = 7,
  u32_eq_builtin = 8,
  u64_eq_builtin = 9,

  i8_gt_builtin = 10,
  i16_gt_builtin = 11,
  i32_gt_builtin = 12,
  i64_gt_builtin = 13,

  u8_gt_builtin = 14,
  u16_gt_builtin = 15,
  u32_gt_builtin = 16,
  u64_gt_builtin = 17,

  i8_gte_builtin = 18,
  i16_gte_builtin = 19,
  i32_gte_builtin = 20,
  i64_gte_builtin = 21,

  u8_gte_builtin = 22,
  u16_gte_builtin = 23,
  u32_gte_builtin = 24,
  u64_gte_builtin = 25,

  i8_lt_builtin = 26,
  i16_lt_builtin = 27,
  i32_lt_builtin = 28,
  i64_lt_builtin = 29,

  u8_lt_builtin = 30,
  u16_lt_builtin = 31,
  u32_lt_builtin = 32,
  u64_lt_builtin = 33,

  i8_lte_builtin = 34,
  i16_lte_builtin = 35,
  i32_lte_builtin = 36,
  i64_lte_builtin = 37,

  u8_lte_builtin = 38,
  u16_lte_builtin = 39,
  u32_lte_builtin = 40,
  u64_lte_builtin = 41,

  i8_add_builtin = 42,
  i16_add_builtin = 43,
  i32_add_builtin = 44,
  i64_add_builtin = 45,

  u8_add_builtin = 46,
  u16_add_builtin = 47,
  u32_add_builtin = 48,
  u64_add_builtin = 49,

  i8_sub_builtin = 50,
  i16_sub_builtin = 51,
  i32_sub_builtin = 52,
  i64_sub_builtin = 53,

  u8_sub_builtin = 54,
  u16_sub_builtin = 55,
  u32_sub_builtin = 56,
  u64_sub_builtin = 57,

  i8_mul_builtin = 58,
  i16_mul_builtin = 59,
  i32_mul_builtin = 60,
  i64_mul_builtin = 61,

  u8_mul_builtin = 62,
  u16_mul_builtin = 63,
  u32_mul_builtin = 64,
  u64_mul_builtin = 65,

  i8_div_builtin = 66,
  i16_div_builtin = 67,
  i32_div_builtin = 68,
  i64_div_builtin = 69,

  u8_div_builtin = 70,
  u16_div_builtin = 71,
  u32_div_builtin = 72,
  u64_div_builtin = 73,

  i8_rem_builtin = 74,
  i16_rem_builtin = 75,
  i32_rem_builtin = 76,
  i64_rem_builtin = 77,

  u8_rem_builtin = 78,
  u16_rem_builtin = 79,
  u32_rem_builtin = 80,
  u64_rem_builtin = 81,

  // mod_floor
  i8_mod_builtin = 82,
  i16_mod_builtin = 83,
  i32_mod_builtin = 84,
  i64_mod_builtin = 85,

  builtin_term_amount = 86,
} builtin_term;

extern const char *builtin_type_names[];
extern const type builtin_types[];
extern const node_ind_t named_builtin_type_amount;
extern const node_ind_t builtin_type_amount;
extern const node_ind_t builtin_type_inds[];
extern const node_ind_t builtin_type_ind_amount;
extern const char *builtin_term_names[];

#define EACH_BUILTIN_BITWIDTH_CASE(prefix, suffix)                             \
  prefix##8##suffix : case prefix##16##suffix : case prefix##32##suffix        \
    : case prefix##64##suffix

#define EACH_SIGNED_BUILTIN_CASE(op)                                           \
  EACH_BUILTIN_BITWIDTH_CASE(i, _##op##_builtin)
#define EACH_UNSIGNED_BUILTIN_CASE(op)                                         \
  EACH_BUILTIN_BITWIDTH_CASE(u, _##op##_builtin)

#define EACH_BUILTIN_TYPE_CASE(op)                                             \
  EACH_SIGNED_BUILTIN_CASE(op) : case EACH_UNSIGNED_BUILTIN_CASE(op)

#define BUILTIN_ADD_CASES EACH_BUILTIN_TYPE_CASE(add)
#define BUILTIN_SUB_CASES EACH_BUILTIN_TYPE_CASE(sub)
#define BUILTIN_MUL_CASES EACH_BUILTIN_TYPE_CASE(mul)

#define BUILTIN_IDIV_CASES EACH_SIGNED_BUILTIN_CASE(div)
#define BUILTIN_IREM_CASES EACH_SIGNED_BUILTIN_CASE(rem)

#define BUILTIN_UDIV_CASES EACH_UNSIGNED_BUILTIN_CASE(div)
#define BUILTIN_UREM_CASES EACH_UNSIGNED_BUILTIN_CASE(rem)

#define BUILTIN_DIV_CASES EACH_BUILTIN_TYPE_CASE(div)
#define BUILTIN_REM_CASES EACH_BUILTIN_TYPE_CASE(rem)
#define BUILTIN_MOD_CASES EACH_SIGNED_BUILTIN_CASE(mod)

#define BUILTIN_LT_CASES EACH_BUILTIN_TYPE_CASE(lt)
#define BUILTIN_LTE_CASES EACH_BUILTIN_TYPE_CASE(lte)
#define BUILTIN_GT_CASES EACH_BUILTIN_TYPE_CASE(gt)
#define BUILTIN_GTE_CASES EACH_BUILTIN_TYPE_CASE(gte)
#define BUILTIN_EQ_CASES EACH_BUILTIN_TYPE_CASE(eq)

#define BUILTIN_COMPARISON_CASES                                               \
  BUILTIN_LT_CASES:                                                            \
  case BUILTIN_LTE_CASES:                                                      \
  case BUILTIN_GT_CASES:                                                       \
  case BUILTIN_GTE_CASES:                                                      \
  case BUILTIN_EQ_CASES

#define BUILTIN_ARITHMETIC_CASES                                               \
  BUILTIN_ADD_CASES:                                                           \
  case BUILTIN_SUB_CASES:                                                      \
  case BUILTIN_MUL_CASES:                                                      \
  case BUILTIN_DIV_CASES:                                                      \
  case BUILTIN_REM_CASES:                                                      \
  case BUILTIN_MOD_CASES

#define BUILTIN_BINOP_CASES                                                    \
  BUILTIN_COMPARISON_CASES:                                                    \
  case BUILTIN_ARITHMETIC_CASES

#define BUILTIN_FUNCTION_CASES                                                 \
  BUILTIN_COMPARISON_CASES:                                                    \
  case BUILTIN_ARITHMETIC_CASES

#define BUILTIN_NON_FUNCTION_CASES                                             \
  true_builtin:                                                                \
  case false_builtin:                                                          \
  case builtin_term_amount
