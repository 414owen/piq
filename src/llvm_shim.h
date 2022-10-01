#pragma once

#include <llvm-c/Core.h>

LLVMValueRef LLVMAddFunctionCustom(LLVMModuleRef M, const char *name,
                                   size_t name_size, LLVMTypeRef FunctionTy);
