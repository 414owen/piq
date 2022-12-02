#pragma once

#include "types.h"

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>

extern const char *builtin_type_names[];
extern const type builtin_types[];
extern const node_ind_t builtin_type_amount;
extern const node_ind_t builtin_type_inds[];
extern const node_ind_t builtin_type_ind_amount;
extern const char *builtin_term_names[];
extern const node_ind_t builtin_term_amount;
extern const node_ind_t builtin_term_type_inds[];

extern LLVMValueRef builtin_term_llvm_values[];

// called to initialise the above
void init_builtins_module(void);
