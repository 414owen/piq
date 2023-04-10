// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "assert.h"
#include "builtins.h"
#include "consts.h"
#include "types.h"
#include "util.h"

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>

// When adding a builtin, you'll need to add it to a couple of places:
// * llvm_builtin_predicates
// * cg_builtin_to_value
// * macros in builtins.h

const char *builtin_type_names[] = {
  [bool_type_ind] = "Bool",
  [u8_type_ind] = "U8",
  [u16_type_ind] = "U16",
  [u32_type_ind] = "U32",
  [u64_type_ind] = "U64",
  [i8_type_ind] = "I8",
  [i16_type_ind] = "I16",
  [i32_type_ind] = "I32",
  [i64_type_ind] = "I64",
  [string_type_ind] = "String",
};

enum {
  // index into the type indices array
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

  any_int_type_ind_start = builtin_term_amount + 48,

  derived_type_amount = builtin_term_amount + 56
};

const node_ind_t named_builtin_type_amount = STATIC_LEN(builtin_type_names);

const type builtin_types[] = {
  [bool_type_ind] =
    {
      .tag = T_BOOL,
      .sub_a = 0,
      .sub_b = 0,
    },
  [u8_type_ind] =
    {
      .tag = T_U8,
      .sub_a = 0,
      .sub_b = 0,
    },
  [u16_type_ind] =
    {
      .tag = T_U16,
      .sub_a = 0,
      .sub_b = 0,
    },
  [u32_type_ind] =
    {
      .tag = T_U32,
      .sub_a = 0,
      .sub_b = 0,
    },
  [u64_type_ind] =
    {
      .tag = T_U64,
      .sub_a = 0,
      .sub_b = 0,
    },
  [i8_type_ind] =
    {
      .tag = T_I8,
      .sub_a = 0,
      .sub_b = 0,
    },
  [i16_type_ind] =
    {
      .tag = T_I16,
      .sub_a = 0,
      .sub_b = 0,
    },
  [i32_type_ind] =
    {
      .tag = T_I32,
      .sub_a = 0,
      .sub_b = 0,
    },
  [i64_type_ind] =
    {
      .tag = T_I64,
      .sub_a = 0,
      .sub_b = 0,
    },
  [string_type_ind] =
    {
      .tag = T_LIST,
      .sub_a = u8_type_ind,
      .sub_b = 0,
    },

  [compare_i8s_type_ind] =
    {
      .tag = T_FN,
      .subs_start = i8_predicate_fn_ind_start,
      .sub_amt = 3,
    },

  [compare_i16s_type_ind] =
    {
      .tag = T_FN,
      .subs_start = i16_predicate_fn_ind_start,
      .sub_amt = 3,
    },

  [compare_i32s_type_ind] =
    {
      .tag = T_FN,
      .subs_start = i32_predicate_fn_ind_start,
      .sub_amt = 3,
    },

  [compare_i64s_type_ind] =
    {
      .tag = T_FN,
      .subs_start = i64_predicate_fn_ind_start,
      .sub_amt = 3,
    },

  [i8_arithmetic_type_ind] =
    {
      .tag = T_FN,
      .subs_start = i8_arithmetic_fn_ind_start,
      .sub_amt = 3,
    },

  [i16_arithmetic_type_ind] =
    {
      .tag = T_FN,
      .subs_start = i16_arithmetic_fn_ind_start,
      .sub_amt = 3,
    },

  [i32_arithmetic_type_ind] =
    {
      .tag = T_FN,
      .subs_start = i32_arithmetic_fn_ind_start,
      .sub_amt = 3,
    },

  [i64_arithmetic_type_ind] =
    {
      .tag = T_FN,
      .subs_start = i64_arithmetic_fn_ind_start,
      .sub_amt = 3,
    },

  [any_int_type_ind] =
    {
      .check_tag = TC_OR,
      .subs_start = any_int_type_ind_start,
      // signed and usigned 8, 16, 32, and 64 bits
      .sub_amt = 8,
    },
};

const node_ind_t builtin_type_amount = STATIC_LEN(builtin_types);

const char *builtin_term_names[builtin_term_amount] = {
  [true_builtin] = "True",        [false_builtin] = "False",

  [i8_eq_builtin] = "i8-eq?",     [i16_eq_builtin] = "i16-eq?",
  [i32_eq_builtin] = "i32-eq?",   [i64_eq_builtin] = "i64-eq?",

  [u8_eq_builtin] = "u8-eq?",     [u16_eq_builtin] = "u16-eq?",
  [u32_eq_builtin] = "u32-eq?",   [u64_eq_builtin] = "u64-eq?",

  [i8_gt_builtin] = "i8-gt?",     [i16_gt_builtin] = "i16-gt?",
  [i32_gt_builtin] = "i32-gt?",   [i64_gt_builtin] = "i64-gt?",

  [u8_gt_builtin] = "u8-gt?",     [u16_gt_builtin] = "u16-gt?",
  [u32_gt_builtin] = "u32-gt?",   [u64_gt_builtin] = "u64-gt?",

  [i8_gte_builtin] = "i8-gte?",   [i16_gte_builtin] = "i16-gte?",
  [i32_gte_builtin] = "i32-gte?", [i64_gte_builtin] = "i64-gte?",

  [u8_gte_builtin] = "u8-gte?",   [u16_gte_builtin] = "u16-gte?",
  [u32_gte_builtin] = "u32-gte?", [u64_gte_builtin] = "u64-gte?",

  [i8_lt_builtin] = "i8-lt?",     [i16_lt_builtin] = "i16-lt?",
  [i32_lt_builtin] = "i32-lt?",   [i64_lt_builtin] = "i64-lt?",

  [u8_lt_builtin] = "u8-lt?",     [u16_lt_builtin] = "u16-lt?",
  [u32_lt_builtin] = "u32-lt?",   [u64_lt_builtin] = "u64-lt?",

  [i8_lte_builtin] = "i8-lte?",   [i16_lte_builtin] = "i16-lte?",
  [i32_lte_builtin] = "i32-lte?", [i64_lte_builtin] = "i64-lte?",

  [u8_lte_builtin] = "u8-lte?",   [u16_lte_builtin] = "u16-lte?",
  [u32_lte_builtin] = "u32-lte?", [u64_lte_builtin] = "u64-lte?",

  [i8_add_builtin] = "i8-add",    [i16_add_builtin] = "i16-add",
  [i32_add_builtin] = "i32-add",  [i64_add_builtin] = "i64-add",

  [u8_add_builtin] = "u8-add",    [u16_add_builtin] = "u16-add",
  [u32_add_builtin] = "u32-add",  [u64_add_builtin] = "u64-add",

  [i8_sub_builtin] = "i8-sub",    [i16_sub_builtin] = "i16-sub",
  [i32_sub_builtin] = "i32-sub",  [i64_sub_builtin] = "i64-sub",

  [u8_sub_builtin] = "u8-sub",    [u16_sub_builtin] = "u16-sub",
  [u32_sub_builtin] = "u32-sub",  [u64_sub_builtin] = "u64-sub",

  [i8_mul_builtin] = "i8-mul",    [i16_mul_builtin] = "i16-mul",
  [i32_mul_builtin] = "i32-mul",  [i64_mul_builtin] = "i64-mul",

  [u8_mul_builtin] = "u8-mul",    [u16_mul_builtin] = "u16-mul",
  [u32_mul_builtin] = "u32-mul",  [u64_mul_builtin] = "u64-mul",

  [i8_div_builtin] = "i8-div",    [i16_div_builtin] = "i16-div",
  [i32_div_builtin] = "i32-div",  [i64_div_builtin] = "i64-div",

  [u8_div_builtin] = "u8-div",    [u16_div_builtin] = "u16-div",
  [u32_div_builtin] = "u32-div",  [u64_div_builtin] = "u64-div",

  [i8_rem_builtin] = "i8-rem",    [i16_rem_builtin] = "i16-rem",
  [i32_rem_builtin] = "i32-rem",  [i64_rem_builtin] = "i64-rem",

  [u8_rem_builtin] = "u8-rem",    [u16_rem_builtin] = "u16-rem",
  [u32_rem_builtin] = "u32-rem",  [u64_rem_builtin] = "u64-rem",

  [i8_mod_builtin] = "i8-mod",    [i16_mod_builtin] = "i16-mod",
  [i32_mod_builtin] = "i32-mod",  [i64_mod_builtin] = "i64-mod",
};

enum {
  builtin_type_ind_amount_enum = builtin_term_amount + derived_type_amount,
};

const type_ref builtin_type_ind_amount = builtin_type_ind_amount_enum;

// builtin_type_inds happens to map terms to their type inds
// and at the end, contains everything we need to represent the
// types at those indices
const type_ref builtin_type_inds[builtin_type_ind_amount_enum] = {
  [true_builtin] = bool_type_ind,
  [false_builtin] = bool_type_ind,

  [i8_eq_builtin] = compare_i8s_type_ind,
  [i16_eq_builtin] = compare_i16s_type_ind,
  [i32_eq_builtin] = compare_i32s_type_ind,
  [i64_eq_builtin] = compare_i64s_type_ind,

  [u8_eq_builtin] = compare_u8s_type_ind,
  [u16_eq_builtin] = compare_u16s_type_ind,
  [u32_eq_builtin] = compare_u32s_type_ind,
  [u64_eq_builtin] = compare_u64s_type_ind,

  [i8_gt_builtin] = compare_i8s_type_ind,
  [i16_gt_builtin] = compare_i16s_type_ind,
  [i32_gt_builtin] = compare_i32s_type_ind,
  [i64_gt_builtin] = compare_i64s_type_ind,

  [u8_gt_builtin] = compare_u8s_type_ind,
  [u16_gt_builtin] = compare_u16s_type_ind,
  [u32_gt_builtin] = compare_u32s_type_ind,
  [u64_gt_builtin] = compare_u64s_type_ind,

  [i8_gte_builtin] = compare_i8s_type_ind,
  [i16_gte_builtin] = compare_i16s_type_ind,
  [i32_gte_builtin] = compare_i32s_type_ind,
  [i64_gte_builtin] = compare_i64s_type_ind,

  [u8_gte_builtin] = compare_u8s_type_ind,
  [u16_gte_builtin] = compare_u16s_type_ind,
  [u32_gte_builtin] = compare_u32s_type_ind,
  [u64_gte_builtin] = compare_u64s_type_ind,

  [i8_lt_builtin] = compare_i8s_type_ind,
  [i16_lt_builtin] = compare_i16s_type_ind,
  [i32_lt_builtin] = compare_i32s_type_ind,
  [i64_lt_builtin] = compare_i64s_type_ind,

  [u8_lt_builtin] = compare_u8s_type_ind,
  [u16_lt_builtin] = compare_u16s_type_ind,
  [u32_lt_builtin] = compare_u32s_type_ind,
  [u64_lt_builtin] = compare_u64s_type_ind,

  [i8_lte_builtin] = compare_i8s_type_ind,
  [i16_lte_builtin] = compare_i16s_type_ind,
  [i32_lte_builtin] = compare_i32s_type_ind,
  [i64_lte_builtin] = compare_i64s_type_ind,

  [u8_lte_builtin] = compare_u8s_type_ind,
  [u16_lte_builtin] = compare_u16s_type_ind,
  [u32_lte_builtin] = compare_u32s_type_ind,
  [u64_lte_builtin] = compare_u64s_type_ind,

  [i8_add_builtin] = i8_arithmetic_type_ind,
  [i16_add_builtin] = i16_arithmetic_type_ind,
  [i32_add_builtin] = i32_arithmetic_type_ind,
  [i64_add_builtin] = i64_arithmetic_type_ind,

  [u8_add_builtin] = u8_arithmetic_type_ind,
  [u16_add_builtin] = u16_arithmetic_type_ind,
  [u32_add_builtin] = u32_arithmetic_type_ind,
  [u64_add_builtin] = u64_arithmetic_type_ind,

  [i8_sub_builtin] = i8_arithmetic_type_ind,
  [i16_sub_builtin] = i16_arithmetic_type_ind,
  [i32_sub_builtin] = i32_arithmetic_type_ind,
  [i64_sub_builtin] = i64_arithmetic_type_ind,

  [u8_sub_builtin] = u8_arithmetic_type_ind,
  [u16_sub_builtin] = u16_arithmetic_type_ind,
  [u32_sub_builtin] = u32_arithmetic_type_ind,
  [u64_sub_builtin] = u64_arithmetic_type_ind,

  [i8_mul_builtin] = i8_arithmetic_type_ind,
  [i16_mul_builtin] = i16_arithmetic_type_ind,
  [i32_mul_builtin] = i32_arithmetic_type_ind,
  [i64_mul_builtin] = i64_arithmetic_type_ind,

  [u8_mul_builtin] = u8_arithmetic_type_ind,
  [u16_mul_builtin] = u16_arithmetic_type_ind,
  [u32_mul_builtin] = u32_arithmetic_type_ind,
  [u64_mul_builtin] = u64_arithmetic_type_ind,

  [i8_div_builtin] = i8_arithmetic_type_ind,
  [i16_div_builtin] = i16_arithmetic_type_ind,
  [i32_div_builtin] = i32_arithmetic_type_ind,
  [i64_div_builtin] = i64_arithmetic_type_ind,

  [u8_div_builtin] = u8_arithmetic_type_ind,
  [u16_div_builtin] = u16_arithmetic_type_ind,
  [u32_div_builtin] = u32_arithmetic_type_ind,
  [u64_div_builtin] = u64_arithmetic_type_ind,

  [i8_rem_builtin] = i8_arithmetic_type_ind,
  [i16_rem_builtin] = i16_arithmetic_type_ind,
  [i32_rem_builtin] = i32_arithmetic_type_ind,
  [i64_rem_builtin] = i64_arithmetic_type_ind,

  [u8_rem_builtin] = u8_arithmetic_type_ind,
  [u16_rem_builtin] = u16_arithmetic_type_ind,
  [u32_rem_builtin] = u32_arithmetic_type_ind,
  [u64_rem_builtin] = u64_arithmetic_type_ind,

  [i8_mod_builtin] = i8_arithmetic_type_ind,
  [i16_mod_builtin] = i16_arithmetic_type_ind,
  [i32_mod_builtin] = i32_arithmetic_type_ind,
  [i64_mod_builtin] = i64_arithmetic_type_ind,

  [i8_arithmetic_fn_ind_start + 0] = i8_type_ind,
  [i8_arithmetic_fn_ind_start + 1] = i8_type_ind,
  [i8_arithmetic_fn_ind_start + 2] = i8_type_ind,

  [i16_arithmetic_fn_ind_start + 0] = i16_type_ind,
  [i16_arithmetic_fn_ind_start + 1] = i16_type_ind,
  [i16_arithmetic_fn_ind_start + 2] = i16_type_ind,

  [i32_arithmetic_fn_ind_start + 0] = i32_type_ind,
  [i32_arithmetic_fn_ind_start + 1] = i32_type_ind,
  [i32_arithmetic_fn_ind_start + 2] = i32_type_ind,

  [i64_arithmetic_fn_ind_start + 0] = i64_type_ind,
  [i64_arithmetic_fn_ind_start + 1] = i64_type_ind,
  [i64_arithmetic_fn_ind_start + 2] = i64_type_ind,

  [u8_arithmetic_fn_ind_start + 0] = u8_type_ind,
  [u8_arithmetic_fn_ind_start + 1] = u8_type_ind,
  [u8_arithmetic_fn_ind_start + 2] = u8_type_ind,

  [u16_arithmetic_fn_ind_start + 0] = u16_type_ind,
  [u16_arithmetic_fn_ind_start + 1] = u16_type_ind,
  [u16_arithmetic_fn_ind_start + 2] = u16_type_ind,

  [u32_arithmetic_fn_ind_start + 0] = u32_type_ind,
  [u32_arithmetic_fn_ind_start + 1] = u32_type_ind,
  [u32_arithmetic_fn_ind_start + 2] = u32_type_ind,

  [u64_arithmetic_fn_ind_start + 0] = u64_type_ind,
  [u64_arithmetic_fn_ind_start + 1] = u64_type_ind,
  [u64_arithmetic_fn_ind_start + 2] = u64_type_ind,

  [i8_predicate_fn_ind_start + 0] = i8_type_ind,
  [i8_predicate_fn_ind_start + 1] = i8_type_ind,
  [i8_predicate_fn_ind_start + 2] = bool_type_ind,

  [i16_predicate_fn_ind_start + 0] = i16_type_ind,
  [i16_predicate_fn_ind_start + 1] = i16_type_ind,
  [i16_predicate_fn_ind_start + 2] = bool_type_ind,

  [i32_predicate_fn_ind_start + 0] = i32_type_ind,
  [i32_predicate_fn_ind_start + 1] = i32_type_ind,
  [i32_predicate_fn_ind_start + 2] = bool_type_ind,

  [i64_predicate_fn_ind_start + 0] = i64_type_ind,
  [i64_predicate_fn_ind_start + 1] = i64_type_ind,
  [i64_predicate_fn_ind_start + 2] = bool_type_ind,

  [u8_predicate_fn_ind_start + 0] = u8_type_ind,
  [u8_predicate_fn_ind_start + 1] = u8_type_ind,
  [u8_predicate_fn_ind_start + 2] = bool_type_ind,

  [u16_predicate_fn_ind_start + 0] = u16_type_ind,
  [u16_predicate_fn_ind_start + 1] = u16_type_ind,
  [u16_predicate_fn_ind_start + 2] = bool_type_ind,

  [u32_predicate_fn_ind_start + 0] = u32_type_ind,
  [u32_predicate_fn_ind_start + 1] = u32_type_ind,
  [u32_predicate_fn_ind_start + 2] = bool_type_ind,

  [u64_predicate_fn_ind_start + 0] = u64_type_ind,
  [u64_predicate_fn_ind_start + 1] = u64_type_ind,
  [u64_predicate_fn_ind_start + 2] = bool_type_ind,

  [any_int_type_ind_start + 0] = i8_type_ind,
  [any_int_type_ind_start + 1] = i16_type_ind,
  [any_int_type_ind_start + 2] = i32_type_ind,
  [any_int_type_ind_start + 3] = i64_type_ind,
  [any_int_type_ind_start + 4] = u8_type_ind,
  [any_int_type_ind_start + 5] = u16_type_ind,
  [any_int_type_ind_start + 6] = u32_type_ind,
  [any_int_type_ind_start + 7] = u64_type_ind,
};
