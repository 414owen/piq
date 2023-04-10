// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

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
