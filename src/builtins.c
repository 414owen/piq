#include "assert.h"
#include "builtins.h"
#include "consts.h"
#include "types.h"
#include "util.h"

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>

const char *builtin_type_names[] = {
  "Bool",
  "U8",
  "U16",
  "U32",
  "U64",
  "I8",
  "I16",
  "I32",
  "I64",
  "String",
};

const node_ind_t builtin_type_amount = STATIC_LEN(builtin_type_names);

const node_ind_t builtin_type_inds[] = {
  1 // u8
};

const node_ind_t builtin_type_ind_amount = STATIC_LEN(builtin_type_inds);

const type builtin_types[STATIC_LEN(builtin_type_names)] = {
  {
    .tag = T_BOOL,
    .subs_start = 0,
    .sub_amt = 0,
  },
  {
    .tag = T_U8,
    .subs_start = 0,
    .sub_amt = 0,
  },
  {
    .tag = T_U16,
    .subs_start = 0,
    .sub_amt = 0,
  },
  {
    .tag = T_U32,
    .subs_start = 0,
    .sub_amt = 0,
  },
  {
    .tag = T_U64,
    .subs_start = 0,
    .sub_amt = 0,
  },
  {
    .tag = T_I8,
    .subs_start = 0,
    .sub_amt = 0,
  },
  {
    .tag = T_I16,
    .subs_start = 0,
    .sub_amt = 0,
  },
  {
    .tag = T_I32,
    .subs_start = 0,
    .sub_amt = 0,
  },
  {
    .tag = T_I64,
    .subs_start = 0,
    .sub_amt = 0,
  },
  {
    .tag = T_LIST,
    .sub_a = 1,
    .sub_b = 1,
  },

};

const char *builtin_term_names[] = {
  "True",
  "False",
};

const node_ind_t builtin_term_type_inds[STATIC_LEN(builtin_term_names)] = {
  0, // True
  0, // False
};

LLVMValueRef builtin_term_llvm_values[STATIC_LEN(builtin_term_names)];

static void init_llvm(void) {
  LLVMTypeRef bool_type = LLVMInt1Type();
  builtin_term_llvm_values[0] = LLVMConstInt(bool_type, 1, false);
  builtin_term_llvm_values[1] = LLVMConstInt(bool_type, 0, false);
}

void init_builtins_module(void) { init_llvm(); }

const node_ind_t builtin_term_amount = STATIC_LEN(builtin_term_names);
