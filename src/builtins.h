#pragma once

#include "types.h"

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>

typedef enum {
  true_builtin = 0,
  false_builtin = 1,
  i32_eq_builtin = 2,
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
