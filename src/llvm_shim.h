// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <llvm-c/Core.h>

LLVMValueRef LLVMAddFunctionCustom(LLVMModuleRef M, const char *name,
                                   size_t name_size, LLVMTypeRef FunctionTy);
