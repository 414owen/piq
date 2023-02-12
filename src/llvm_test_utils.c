#include <llvm-c/LLJIT.h>
#include <llvm-c/Core.h>

#include "test_llvm.h"
#include "util.h"

llvm_jit_ctx llvm_jit_init(void) {
  LLVMOrcLLJITBuilderRef builder = LLVMOrcCreateLLJITBuilder();

  LLVMOrcLLJITRef jit;
  {
    LLVMErrorRef error = LLVMOrcCreateLLJIT(&jit, builder);

    if (error != LLVMErrorSuccess) {
      char *msg = LLVMGetErrorMessage(error);
      give_up("LLVMOrcCreateLLJIT failed: %s", msg);
    }
  }

  // LLVMOrcExecutionSessionRef session = LLVMOrcLLJITGetExecutionSession(jit);
  LLVMOrcJITDylibRef dylib = LLVMOrcLLJITGetMainJITDylib(jit);
  LLVMOrcThreadSafeContextRef orc_ctx = LLVMOrcCreateNewThreadSafeContext();

  llvm_jit_ctx state = {
    .llvm_ctx = LLVMOrcThreadSafeContextGetContext(orc_ctx),
    .orc_ctx = orc_ctx,
    .jit = jit,
    // .session = session,
    .dylib = dylib,
  };

  return state;
}

void llvm_jit_dispose(llvm_jit_ctx *ctx) {
  LLVMOrcDisposeThreadSafeContext(ctx->orc_ctx);
  LLVMOrcDisposeLLJIT(ctx->jit);
  // This seems to not be needed because the context
  // was breated by LLVMOrcCreateNewThreadSafeContext()
  // which was disposed of above
  // LLVMContextDispose(ctx->llvm_ctx);
}
