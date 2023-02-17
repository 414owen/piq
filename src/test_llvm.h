#pragma once

#include <llvm-c/LLJIT.h>

typedef struct {
  LLVMContextRef llvm_ctx;
  LLVMOrcThreadSafeContextRef orc_ctx;
  LLVMOrcLLJITRef jit;
  LLVMOrcJITDylibRef dylib;
} llvm_jit_ctx;

llvm_jit_ctx llvm_jit_init(void);
void llvm_jit_dispose(llvm_jit_ctx *ctx);
