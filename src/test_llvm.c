#include <stdbool.h>
#include <stdio.h>

#include <llvm-c/LLJIT.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Orc.h>
#include <llvm-c/OrcEE.h>

#include "diagnostic.h"
#include "llvm.h"
#include "parse_tree.h"
#include "test.h"
#include "typecheck.h"
#include "util.h"

typedef struct {
  LLVMContextRef llvm_ctx;
  LLVMOrcThreadSafeContextRef orc_ctx;
  LLVMOrcLLJITRef jit;
  // LLVMOrcExecutionSessionRef session;
  LLVMOrcJITDylibRef dylib;
} jit_ctx;

static jit_ctx jit_llvm_init(void) {
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

  jit_ctx state = {
    .llvm_ctx = LLVMOrcThreadSafeContextGetContext(orc_ctx),
    .orc_ctx = orc_ctx,
    .jit = jit,
    // .session = session,
    .dylib = dylib,
  };

  return state;
}

static void jit_dispose(jit_ctx *ctx) {
  LLVMOrcDisposeLLJIT(ctx->jit);
  LLVMOrcDisposeThreadSafeContext(ctx->orc_ctx);
}

static void test_llvm_produces(test_state *state, const char *input,
                               int32_t expected) {
  jit_ctx ctx = jit_llvm_init();
  parse_tree tree;
  bool success = false;
  tc_res tc = test_upto_typecheck(state, input, &success, &tree);

  source_file test_file = {
    .path = "test_jit",
    .data = input,
  };

  LLVMModuleRef module =
    gen_module(test_file.path, test_file, tree, tc.types, ctx.llvm_ctx);
  LLVMOrcThreadSafeModuleRef tsm =
    LLVMOrcCreateNewThreadSafeModule(module, ctx.orc_ctx);
  LLVMOrcLLJITAddLLVMIRModule(ctx.jit, ctx.dylib, tsm);
  LLVMOrcJITTargetAddress entry_addr;
  {
    LLVMErrorRef error = LLVMOrcLLJITLookup(ctx.jit, &entry_addr, "test");
    if (error != LLVMErrorSuccess) {
      char *msg = LLVMGetErrorMessage(error);
      give_up("LLVMLLJITLookup failed: %s", msg);
    }
  }

  int32_t (*entry)(void) = (int32_t(*)(void))entry_addr;
  int32_t got = entry();
  if (got != expected) {
    failf(state,
          "Jit function returned wrong result. Expected: %d, Got: %d.\n%s",
          expected, got, LLVMPrintModuleToString(module));
  }
  free_parse_tree(tree);
  free_tc_res(tc);
  jit_dispose(&ctx);
}

static void init() {
  LLVMInitializeCore(LLVMGetGlobalPassRegistry());
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
}

void test_llvm(test_state *state) {
  init();

  test_group_start(state, "LLVM");

  test_start(state, "Can return number");
  {
    const char *input = "(sig test (Fn () I32))\n"
                        "(fun test () 2)";

    test_llvm_produces(state, input, 2);
  }
  test_end(state);

  test_start(state, "Returns last number in block");
  {
    const char *input = "(sig test (Fn () I32))\n"
                        "(fun test () (as U8 4) 3)";

    test_llvm_produces(state, input, 3);
  }
  test_end(state);

  test_group_end(state);
}
