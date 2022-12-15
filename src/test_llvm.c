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
      llvm_gen_module(test_file.path, test_file, tree, tc.types, ctx->llvm_ctx);
    add_codegen_timings(state, tree, res);

    ctx->module_str = LLVMPrintModuleToString(res.module);

#ifdef TIME_CODEGEN
    struct timespec start = get_monotonic_time();
#endif
    LLVMOrcThreadSafeModuleRef tsm =
      LLVMOrcCreateNewThreadSafeModule(res.module, ctx->orc_ctx);
    LLVMOrcLLJITAddLLVMIRModule(ctx->jit, ctx->dylib, tsm);
    LLVMErrorRef error = LLVMOrcLLJITLookup(ctx->jit, &entry_addr, "test");

#ifdef TIME_CODEGEN
    struct timespec time_taken = time_since_monotonic(start);
    state->total_codegen_time =
      timespec_add(state->total_codegen_time, time_taken);
#endif

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

static void test_llvm_code_produces_bool(test_state *state,
                                         const char *restrict input,
                                         bool expected) {
  jit_ctx ctx = jit_llvm_init();
  void *entry_addr = get_entry_fn(state, &ctx, input);
  bool (*entry)(void) = (bool (*)(void))entry_addr;
  if (entry) {
    bool got = entry();
    ensure_int_result_matches(state, ctx, expected, got);
  }
  jit_dispose(&ctx);
}

typedef struct {
  int32_t input;
  int32_t expected;
} i32_mapping_test_case;

static void test_llvm_code_maps_int(test_state *state,
                                    const char *restrict input, int case_amt,
                                    i32_mapping_test_case *cases) {
  jit_ctx ctx = jit_llvm_init();
  void *entry_addr = get_entry_fn(state, &ctx, input);
  int32_t (*entry)(int32_t) = (int32_t(*)(int32_t))entry_addr;
  if (entry) {
    for (int i = 0; i < case_amt; i++) {
      i32_mapping_test_case tup = cases[i];
      int32_t got = entry(tup.input);
      ensure_int_result_matches(state, ctx, tup.expected, got);
    }
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
    const char *preamble = "(sig sndpar (Fn I32 I32 I32))\n"
                           "(fun sndpar (a b) b)\n"
                           "\n"
                           "(sig test (Fn I32))\n"
                           "(fun test () ";
    fputs(preamble, source_file.stream);
    for (size_t i = 0; i < depth; i++) {
      fputs("(sndpar 1 ", source_file.stream);
    }
    putc('2', source_file.stream);
    for (size_t i = 0; i < depth; i++) {
      fputc(')', source_file.stream);
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
    const char *input = "(sig test (Fn I32))\n"
                        "(fun test () 2)";

    test_llvm_code_produces_int(state, input, 2);
  }
  test_end(state);

  test_start(state, "Returns last number in block");
  {
    const char *input = "(sig test (Fn I32))\n"
                        "(fun test () (as U8 4) 3)";

    test_llvm_code_produces_int(state, input, 3);
  }
  test_end(state);

  test_start(state, "Can return parameter");
  {
    const char *input = "(sig test (Fn () ()))\n"
                        "(fun test (a) a)";
    test_llvm_code_runs(state, input);
  }
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (a) a)";
    i32_mapping_test_case test_cases[] = {
      {
        .input = 1,
        .expected = 1,
      },
      {
        .input = 2,
        .expected = 2,
      },
      {
        .input = -500,
        .expected = -500,
      },
    };
    test_llvm_code_maps_int(state, input, STATIC_LEN(test_cases), test_cases);
  }
  test_end(state);

  test_start(state, "Can return builtin Bool constructors");
  {
    const char *input = "(sig test (Fn Bool))\n"
                        "(fun test () True)";
    test_llvm_code_produces_bool(state, input, true);
  }
  {
    const char *input = "(sig test (Fn Bool))\n"
                        "(fun test () False)";
    test_llvm_code_produces_bool(state, input, false);
  }
  test_end(state);

  test_start(state, "Can use let bindings");
  {
    const char *input = "(sig test (Fn () ()))\n"
                        "(fun test (()) (let b ()) ())";
    test_llvm_code_runs(state, input);
  }
  {
    const char *input = "(sig test (Fn ()))\n"
                        "(fun test () (let b ()) b)";
    test_llvm_code_runs(state, input);
  }
  {
    const char *input = "(sig test (Fn I32))\n"
                        "(fun test ()\n"
                        "  (sig b I32)\n"
                        "  (let b 3) b)";
    test_llvm_code_produces_int(state, input, 3);
  }
  test_end(state);

  test_start(state, "Two fns and a call");
  {
    const char *input = "(sig test (Fn I32))\n"
                        "(fun test () (a))\n"
                        "(sig a (Fn I32))\n"
                        "(fun a () 2)";

    test_llvm_code_produces_int(state, input, 2);
  }
  test_end(state);

  test_start(state, "If expressions work");
  {
    const char *input = "(sig test (Fn I32))\n"
                        "(fun test () (if True 1 2))";
    test_llvm_code_produces_int(state, input, 1);
  }
  {
    const char *ifthenelse = "(sig ifthenelse (Fn Bool I32 I32 I32))\n"
                             "(fun ifthenelse (cond t f) (if cond t f))\n";
    {
      const char *test = "(sig test (Fn I32))\n"
                         "(fun test () (ifthenelse True 1 2))";
      char *input = join_two(ifthenelse, test);
      test_llvm_code_produces_int(state, input, 1);
      free(input);
    }
  }
  test_end(state);

  test_start(state, "Equality builtin");
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (a) (if (i32-eq? a 8) 4 5))";

    i32_mapping_test_case cases[] = {
      {
        .input = 8,
        .expected = 4,
      },
      {
        .input = 9,
        .expected = 5,
      },
    };
    test_llvm_code_maps_int(state, input, STATIC_LEN(cases), cases);
  }
  test_end(state);

  test_start(state, "Add builtin");
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (a) (i32-add 3 a))";

    i32_mapping_test_case cases[] = {
      {
        .input = 0,
        .expected = 3,
      },
      {
        .input = 8,
        .expected = 11,
      },
      {
        .input = INT32_MAX - 2,
        .expected = INT32_MIN,
      },
      {
        .input = -8,
        .expected = -5,
      },
    };
    test_llvm_code_maps_int(state, input, STATIC_LEN(cases), cases);
  }
  test_end(state);

  test_start(state, "Sub builtin");
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (a) (i32-sub a 4))";

    i32_mapping_test_case cases[] = {
      {
        .input = 4,
        .expected = 0,
      },
      {
        .input = 42,
        .expected = 38,
      },
      {
        .input = INT32_MIN + 2,
        .expected = INT32_MAX - 1,
      },
    };
    test_llvm_code_maps_int(state, input, STATIC_LEN(cases), cases);
  }
  test_end(state);

  test_start(state, "Mul builtin");
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (a) (i32-mul a -3))";

    i32_mapping_test_case cases[] = {
      {
        .input = 1,
        .expected = -3,
      },
      {
        .input = 2,
        .expected = -6,
      },
      {
        .input = -1,
        .expected = 3,
      },
      {
        .input = -3,
        .expected = 9,
      },
    };
    test_llvm_code_maps_int(state, input, STATIC_LEN(cases), cases);
    // TODO test overflow
  }
  test_end(state);

  test_start(state, "Div builtin");
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (a) (i32-div -12 a))";

    i32_mapping_test_case cases[] = {
      {.input = 1, .expected = -12},
      {.input = -1, .expected = 12},
      {.input = 3, .expected = -4},
      {.input = -4, .expected = 3},
    };
    test_llvm_code_maps_int(state, input, STATIC_LEN(cases), cases);
  }
  test_end(state);

  test_start(state, "Rem builtin");
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (a) (i32-rem 10 a))";

    i32_mapping_test_case cases[] = {
      {.input = 1, .expected = 0},
      {.input = 5, .expected = 0},
      {.input = 10, .expected = 0},
      {.input = -4, .expected = 2},
      {.input = -9, .expected = 1},
    };
    test_llvm_code_maps_int(state, input, STATIC_LEN(cases), cases);
  }
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (a) (i32-rem -10 a))";

    i32_mapping_test_case cases[] = {
      {.input = 1, .expected = 0},
      {.input = 5, .expected = 0},
      {.input = 10, .expected = 0},
      {.input = 4, .expected = -2},
      {.input = 9, .expected = -1},
      {.input = -4, .expected = -2},
      {.input = -9, .expected = -1},
    };
    test_llvm_code_maps_int(state, input, STATIC_LEN(cases), cases);
  }
  test_end(state);

  test_start(state, "Mod builtin");
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (a) (i32-mod 10 a))";

    i32_mapping_test_case cases[] = {
      {.input = 1, .expected = 0},
      {.input = 5, .expected = 0},
      {.input = 10, .expected = 0},
      {.input = -4, .expected = -2},
      {.input = -9, .expected = -8},
    };
    test_llvm_code_maps_int(state, input, STATIC_LEN(cases), cases);
  }
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (a) (i32-mod -10 a))";

    i32_mapping_test_case cases[] = {
      {.input = 1, .expected = 0},
      {.input = 5, .expected = 0},
      {.input = 10, .expected = 0},
      {.input = 4, .expected = 2},
      {.input = 9, .expected = 8},
      {.input = -4, .expected = -2},
      {.input = -9, .expected = -1},
    };
    test_llvm_code_maps_int(state, input, STATIC_LEN(cases), cases);
  }
  test_end(state);

  test_start(state, "Mutual recursion works");
  {
    const char *input = "(sig a (Fn I32))\n"
                        "(fun a () (if True (b) 1))"
                        "\n"
                        "(sig b (Fn I32))\n"
                        "(fun b () (if False (a) 2))"
                        "\n"
                        "(sig test (Fn I32))\n"
                        "(fun test () (a))";
    test_llvm_code_produces_int(state, input, 2);
  }
  test_end(state);

  test_start(state, "Can use builtin comparisons as conditions");
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (a)\n"
                        "  (if (i32-gte? a 47) 43 54))";

    i32_mapping_test_case cases[] = {
      {.input = 65, .expected = 43},
      {.input = 23, .expected = 54},
    };
    test_llvm_code_maps_int(state, input, STATIC_LEN(cases), cases);
  }
  test_end(state);

  test_start(state, "Can assign builtin to var");
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (a)\n"
                        "  (let f i32-mul)\n"
                        "  (f 4 a))";

    i32_mapping_test_case cases[] = {
      {.input = 12, .expected = 48},
      {.input = -7, .expected = -28},
    };
    test_llvm_code_maps_int(state, input, STATIC_LEN(cases), cases);
  }
  test_end(state);

  test_group_start(state, "Can use builtins in complex expressions");
  test_start(state, "Results of if-expressions");
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (a)\n"
                        "  (let f (if True i32-add i32-sub))\n"
                        "  (f a 10))";

    i32_mapping_test_case cases[] = {
      {.input = 42, .expected = 52},
      {.input = 56, .expected = 66},
    };
    test_llvm_code_maps_int(state, input, STATIC_LEN(cases), cases);
  }
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (a)\n"
                        "  (let f (if False i32-add i32-sub))\n"
                        "  (f a 10))";

    i32_mapping_test_case cases[] = {
      {.input = 42, .expected = 32},
      {.input = 56, .expected = 46},
    };
    test_llvm_code_maps_int(state, input, STATIC_LEN(cases), cases);
  }
  {
    const char *input = "(sig test (Fn I32 I32))\n"
                        "(fun test (a)\n"
                        "  (let f (if (i32-lte? a 12) i32-add i32-sub))\n"
                        "  (f 3 4))";

    i32_mapping_test_case cases[] = {
      {.input = 11, .expected = 7},
      {.input = 13, .expected = -1},
    };
    test_llvm_code_maps_int(state, input, STATIC_LEN(cases), cases);
  }
  test_end(state);
  test_group_end(state);

  if (!state->config.lite) {
    test_robustness(state);
  }

  test_group_end(state);
}
