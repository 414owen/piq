#pragma once

#include <llvm-c/Core.h>

#include "typecheck.h"

typedef struct {
  LLVMModuleRef module;
#ifdef TIME_CODEGEN
  struct timespec time_taken;
#endif
} llvm_res;

llvm_res gen_module(char *module_name, source_file source, parse_tree tree,
                    type_info types, LLVMContextRef ctx);

void gen_and_print_module(source_file source, parse_tree tree, type_info types,
                          FILE *out);
