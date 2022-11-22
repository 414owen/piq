#include <stdbool.h>
#include <stdio.h>

#include <llvm-c/LLJIT.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Orc.h>
#include <llvm-c/OrcEE.h>

#include "diagnostic.h"
#include "llvm.h"
#include "test.h"
#include "tests.h"
#include "test_upto.h"
#include "util.h"

typedef struct {
  LLVMContextRef llvm_ctx;
  LLVMOrcThreadSafeContextRef orc_ctx;
  LLVMOrcLLJITRef jit;
  char *module_str;
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
  LLVMDisposeMessage(ctx->module_str);
  LLVMOrcDisposeLLJIT(ctx->jit);
  LLVMOrcDisposeThreadSafeContext(ctx->orc_ctx);
}

static void *get_entry_fn(test_state *state, jit_ctx *ctx, const char *input) {
  parse_tree tree;
  bool success = false;
  tc_res tc = test_upto_typecheck(state, input, &success, &tree);

  if (tc.error_amt > 0)
    return NULL;

  LLVMOrcJITTargetAddress entry_addr = (LLVMOrcJITTargetAddress)NULL;

  if (success) {
    source_file test_file = {
      .path = "test_jit",
      .data = input,
    };

    llvm_res res =
      gen_module(test_file.path, test_file, tree, tc.types, ctx->llvm_ctx);
    add_codegen_timings(state, tree, res);

    ctx->module_str = LLVMPrintModuleToString(res.module);

    struct timespec start = get_monotonic_time();
    LLVMOrcThreadSafeModuleRef tsm =
      LLVMOrcCreateNewThreadSafeModule(res.module, ctx->orc_ctx);
    LLVMOrcLLJITAddLLVMIRModule(ctx->jit, ctx->dylib, tsm);
    LLVMErrorRef error = LLVMOrcLLJITLookup(ctx->jit, &entry_addr, "test");
    struct timespec time_taken = time_since_monotonic(start);
    state->total_codegen_time =
      timespec_add(state->total_codegen_time, time_taken);
    if (error != LLVMErrorSuccess) {
      char *msg = LLVMGetErrorMessage(error);
      failf(state,
            "LLVMLLJITLookup failed:\n%s\nIn module:\n%s",
            msg,
            ctx->module_str);
      LLVMDisposeErrorMessage(msg);
    }

    free_parse_tree(tree);
    free_tc_res(tc);
  }
  return (void *)entry_addr;
}

static void ensure_int_result_matches(test_state *state, jit_ctx ctx,
                                      int32_t expected, int32_t got) {
  if (got != expected) {
    failf(
      state,
      "Jit function returned wrong result. Expected: %d, Got: %d.\nModule:%s\n",
      expected,
      got,
      ctx.module_str);
  }
}

static void test_llvm_code_produces_int(test_state *state,
                                        const char *restrict input,
                                        int32_t expected) {
  jit_ctx ctx = jit_llvm_init();
  void *entry_addr = get_entry_fn(state, &ctx, input);
  int32_t (*entry)(void) = (int32_t(*)(void))entry_addr;
  if (entry) {
    int32_t got = entry();
    ensure_int_result_matches(state, ctx, expected, got);
  }
  jit_dispose(&ctx);
}

static void test_llvm_code_maps_int(test_state *state,
                                    const char *restrict input,
                                    int32_t input_param, int32_t expected) {
  jit_ctx ctx = jit_llvm_init();
  void *entry_addr = get_entry_fn(state, &ctx, input);
  int32_t (*entry)(int32_t) = (int32_t(*)(int32_t))entry_addr;
  if (entry) {
    int32_t got = entry(input_param);
    ensure_int_result_matches(state, ctx, expected, got);
  }
  jit_dispose(&ctx);
}

static void test_llvm_code_runs(test_state *state, const char *restrict input) {
  jit_ctx ctx = jit_llvm_init();
  void *entry_addr = get_entry_fn(state, &ctx, input);
  int32_t (*entry)(void) = (int32_t(*)(void))entry_addr;
  if (entry) {
    entry();
  }
  jit_dispose(&ctx);
}

static void init() {
  LLVMInitializeCore(LLVMGetGlobalPassRegistry());
  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
}

static void test_robustness(test_state *state) {
  test_group_start(state, "Robustness");

  test_start(state, "Nested calls");
  {
    stringstream source_file;
    ss_init_immovable(&source_file);
    static const size_t depth = 1000;
    const char *preamble = "(sig sndpar (Fn (I32, I32) I32))\n"
                           "(fun sndpar (a, b) b)\n"
                           "\n"
                           "(sig test (Fn () I32))\n"
                           "(fun test () ";
    fputs(preamble, source_file.stream);
    for (size_t i = 0; i < depth; i++) {
      fputs("(sndpar (1, ", source_file.stream);
    }
    putc('2', source_file.stream);
    for (size_t i = 0; i < depth; i++) {
      fputs("))", source_file.stream);
    }
    putc(')', source_file.stream);
    ss_finalize(&source_file);

    test_llvm_code_produces_int(state, source_file.string, 2);

    free(source_file.string);
  }
  test_end(state);

  test_group_end(state);
}

void test_llvm(test_state *state) {
  init();

  test_group_start(state, "LLVM");

  test_start(state, "Can return number");
  {
    const char *input = "(sig test (Fn () I32))\n"
                        "(fun test () 2)";

    test_llvm_code_produces_int(state, input, 2);
  }
  test_end(state);

  test_start(state, "Returns last number in block");
  {
    const char *input = "(sig test (Fn () I32))\n"
                        "(fun test () (as U8 4) 3)";

    test_llvm_code_produces_int(state, input, 3);
  }
  test_end(state);

  test_start(state, "Can return parameter");
  {
    const char *input = "(sig test (Fn () ()))\n"
                        "(fun test a a)";
    test_llvm_code_runs(state, input);
  }
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test a a)";
    test_llvm_code_maps_int(state, input, 1, 1);
    test_llvm_code_maps_int(state, input, 2, 2);
  }
  test_end(state);

  test_start(state, "Can use let bindings");
  {
    const char *input = "(sig test (Fn () ()))\n"
                        "(fun test () (let b ()) ())";
    test_llvm_code_runs(state, input);
  }
  /*
  {
    const char *input = "(sig test (Fn () I32))\n"
                        "(fun test ()\n"
                        "  (sig b I32)\n"
                        "  (let b 3) b)";
    test_llvm_code_produces_int(state, input, 3);
  }
  */
  test_end(state);

  test_start(state, "If expressions work")
  {
    const char *ifthenelse = "(sig ifthenelse (Fn (Bool, I32, I32) I32))\n"
                             "(fun ifthenelse (cond, t, f) (if cond t f))\n";
    {
      const char *test = "(sig test (Fn () I32))\n"
                         "(fun test () (ifthenelse (True, 1, 2)))";
      char *input = join_two(ifthenelse, test);
      test_llvm_code_produces_int(state, input, 1);
      free(input);
    }
  }
  test_end(state);

  /*
  test_start(state, "Two fns and a call");
  {
    const char *input = "(sig test (Fn () ()))\n"
                        "(fun test () (a ()))\n"
                        "(sig a (Fn () ()))\n"
                        "(fun a () ())";

    test_llvm_code_produces_int(state, input, 2);
  }
  test_end(state);
  */

  test_robustness(state);

  test_group_end(state);
}
