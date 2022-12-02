#include "assert.h"
#include "builtins.h"
#include "consts.h"
#include "types.h"
#include "util.h"

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
  i32_eq_type_ind = 10,
  tup_of_i32_type_ind = 11,
};

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

const node_ind_t named_builtin_type_amount = STATIC_LEN(builtin_type_names);

const type builtin_types[] = {
  [bool_type_ind] = { .tag = T_BOOL, .sub_a = 0, .sub_b = 0,},
  [u8_type_ind] = { .tag = T_U8, .sub_a = 0, .sub_b = 0,},
  [u16_type_ind] = { .tag = T_U16, .sub_a = 0, .sub_b = 0,},
  [u32_type_ind] = { .tag = T_U32, .sub_a = 0, .sub_b = 0,},
  [u64_type_ind] = { .tag = T_U64, .sub_a = 0, .sub_b = 0,},
  [i8_type_ind] = { .tag = T_I8, .sub_a = 0, .sub_b = 0,},
  [i16_type_ind] = { .tag = T_I16, .sub_a = 0, .sub_b = 0,},
  [i32_type_ind] = { .tag = T_I32, .sub_a = 0, .sub_b = 0,},
  [i64_type_ind] = { .tag = T_I64, .sub_a = 0, .sub_b = 0,},
  [string_type_ind] = { .tag = T_LIST, .sub_a = u8_type_ind, .sub_b = 0,},
  [i32_eq_type_ind] = { .tag = T_FN, .sub_a = tup_of_i32_type_ind, .sub_b = bool_type_ind,},
};

const node_ind_t builtin_type_amount = STATIC_LEN(builtin_types);

const char *builtin_term_names[] = {
  [true_builtin] = "True",
  [false_builtin] = "False",
  [i32_eq_builtin] = "i32-eq?",
};

const node_ind_t builtin_term_type_inds[STATIC_LEN(builtin_term_names)] = {
  [true_builtin] = bool_type_ind, // True
  [false_builtin] = bool_type_ind, // False
  [i32_eq_builtin] = i32_eq_type_ind, // TODO
};

const node_ind_t builtin_term_amount = STATIC_LEN(builtin_term_names);
