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
  LLVMOrcExecutionSessionRef session;
  LLVMOrcJITDylibRef dylib;

  LLVMTargetRef target;
  LLVMTargetMachineRef target_machine;
} jit_ctx;

static jit_ctx jit_llvm_init(void) {
  LLVMInitializeCore(LLVMGetGlobalPassRegistry());
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMOrcLLJITBuilderRef builder = LLVMOrcCreateLLJITBuilder();

  LLVMOrcLLJITRef jit;
  {
    LLVMErrorRef error = LLVMOrcCreateLLJIT(&jit, builder);

    if (error != LLVMErrorSuccess) {
      char *msg = LLVMGetErrorMessage(error);
      give_up("LLVMOrcCreateLLJIT failed: %s", msg);
    }
  }

  LLVMOrcExecutionSessionRef session = LLVMOrcLLJITGetExecutionSession(jit);
  LLVMOrcJITDylibRef dylib = LLVMOrcLLJITGetMainJITDylib(jit);
  LLVMOrcThreadSafeContextRef orc_ctx = LLVMOrcCreateNewThreadSafeContext();

  char *triple = LLVMGetDefaultTargetTriple();
  char *error;
  LLVMTargetRef target;
  if (LLVMGetTargetFromTriple(triple, &target, &error)) {
    give_up("Failed to get LLVM target for %s: %s", triple, error);
  }

  LLVMTargetMachineRef target_machine =
    LLVMCreateTargetMachine(target, triple, "", "", LLVMCodeGenLevelDefault,
                            LLVMRelocDefault, LLVMCodeModelJITDefault);

  LLVMDisposeMessage(triple);
  assert(target_machine);

  jit_ctx state = {
    .llvm_ctx = LLVMOrcThreadSafeContextGetContext(orc_ctx),
    .orc_ctx = orc_ctx,
    .jit = jit,
    .session = session,
    .dylib = dylib,

    .target = target,
    .target_machine = target_machine,
  };

  return state;
}

static void test_llvm_produces(test_state *state, jit_ctx *ctx,
                               const char *input, int32_t expected) {
  parse_tree tree;
  bool success = false;
  tc_res tc = test_upto_typecheck(state, input, &success, &tree);

  source_file test_file = {
    .path = "test_jit",
    .data = input,
  };

  LLVMModuleRef module =
    gen_module(test_file.path, test_file, tree, tc.types, ctx->llvm_ctx);
  LLVMOrcThreadSafeModuleRef tsm =
    LLVMOrcCreateNewThreadSafeModule(module, ctx->orc_ctx);
  LLVMOrcLLJITAddLLVMIRModule(ctx->jit, ctx->dylib, tsm);
  LLVMOrcJITTargetAddress entry_addr;
  {
    LLVMErrorRef error = LLVMOrcLLJITLookup(ctx->jit, &entry_addr, "test");
    if (error != LLVMErrorSuccess) {
      char *msg = LLVMGetErrorMessage(error);
      give_up("LLVMLLJITLookup failed: %s", msg);
    }
  }

  int32_t (*entry)(void) = (int32_t(*)(void))entry_addr;
  int32_t got = entry();
  if (got != expected) {
    failf(state, "Jit function returned wrong result. Expected: %d, Got: %d",
          expected, got);
  }
}

void test_llvm(test_state *state) {
  jit_ctx ctx = jit_llvm_init();
  test_group_start(state, "LLVM");

  test_start(state, "Can return number");
  {
    const char *input = "(sig test (Fn () I32))\n"
                        "(fun test () 2)";

    test_llvm_produces(state, &ctx, input, 2);
  }
  test_end(state);

  /*
  test_start(state, "Returns last number in block");
  {
    const char *input = "(sig test (Fn () I32))\n"
                        "(fun test () 2 3)";

    test_llvm_produces(state, &ctx, input, 2);
  }
  test_end(state);
  */

  test_group_end(state);
}
