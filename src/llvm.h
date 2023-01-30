#pragma once

#include <llvm-c/Core.h>

#include "typecheck.h"

typedef struct {
  LLVMModuleRef module;
#ifdef TIME_CODEGEN
  struct timespec time_taken;
#endif
} llvm_res;

LLVMTypeRef llvm_construct_type_internal(LLVMTypeRef *llvm_type_refs, type_tree types, LLVMContextRef context, node_ind_t root_type_ind);

llvm_res llvm_gen_module(const char *module_name, source_file source,
                         parse_tree tree, type_info types, LLVMContextRef ctx);

void gen_and_print_module(source_file source, parse_tree tree, type_info types,
                          FILE *out);
