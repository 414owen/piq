#include "assert.h"
#include "builtins.h"
#include "consts.h"
#include "types.h"
#include "util.h"

const char *builtin_type_names[] = {
  "Bool", "U8", "U16", "U32", "U64", "I8", "I16", "I32", "I64", "String",
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
    .sub_a = 0,
    .sub_b = 0,
  },
};

const char *builtin_terms[] = {
  "True",
  "False",
};

const node_ind_t builtin_term_amount = STATIC_LEN(builtin_terms);
