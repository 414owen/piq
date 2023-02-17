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
#include "test_llvm.h"
#include "util.h"
#include "typedefs.h"

static char *ensure_int_result_matches(int32_t expected, int32_t got) {
  char *res = NULL;
  if (HEDLEY_UNLIKELY(got != expected)) {
    res = format_to_string("Expected: %d, Got: %d.", expected, got);
  }
  return res;
}

typedef uint32_t (*unit_to_int)(void);
char *test_produces_int_callback(unit_to_int f, uint32_t *expected) {
  return ensure_int_result_matches(f(), *expected);
}

static void test_llvm_code_produces_int(test_state *state,
                                        const char *restrict input,
                                        int32_t expected) {
  llvm_symbol_test tests[] = {
    {
      .symbol_name = "test",
      .test_callback = (voidfn)test_produces_int_callback,
      .data = &expected,
    },
  };
  test_upto_codegen_with(state, input, tests, STATIC_LEN(tests));
}

typedef struct {
  int32_t input;
  int32_t expected;
} i32_mapping_test_case;

typedef struct {
  int case_amt;
  i32_mapping_test_case *cases;
} i32_mapping_test_cases;

typedef uint32_t (*int_to_int)(uint32_t);
char *test_maps_int_callback(int_to_int f, i32_mapping_test_case *data) {
  return ensure_int_result_matches(f(data->input), data->expected);
}

static char *test_maps_ints_callback(int_to_int f,
                                     i32_mapping_test_cases *data_p) {
  i32_mapping_test_cases data = *data_p;
  for (int i = 0; i < data.case_amt; i++) {
    i32_mapping_test_case test_case = data.cases[i];
    char *err =
      ensure_int_result_matches(f(test_case.input), test_case.expected);
    if (err != NULL) {
      return err;
    }
  }
  return NULL;
}

static void test_llvm_code_maps_ints(test_state *state,
                                     const char *restrict input, int case_amt,
                                     i32_mapping_test_case *cases) {
  i32_mapping_test_cases test_cases = {
    .case_amt = case_amt,
    .cases = cases,
  };
  llvm_symbol_test tests[] = {
    {
      .symbol_name = "test",
      .test_callback = (voidfn)test_maps_ints_callback,
      .data = &test_cases,
    },
  };
  test_upto_codegen_with(state, input, tests, STATIC_LEN(tests));
}

typedef bool (*unit_to_bool)(void);
char *test_produces_bool_callback(unit_to_bool f, bool *data) {
  return ensure_int_result_matches(f(), *data);
}

static void test_llvm_code_produces_bool(test_state *state,
                                         const char *restrict input,
                                         bool expected) {
  llvm_symbol_test test = {.symbol_name = "test",
                           .test_callback = (voidfn)test_produces_bool_callback,
                           .data = &expected};
  test_upto_codegen_with(state, input, &test, 1);
}

char *test_code_runs_callback(voidfn f, void *data) {
  f();
  return NULL;
}

static void test_llvm_code_runs(test_state *state, const char *restrict input) {
  llvm_symbol_test test = {
    .symbol_name = "test",
    .test_callback = (voidfn)test_code_runs_callback,
  };
  test_upto_codegen_with(state, input, &test, 1);
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
    test_llvm_code_maps_ints(state, input, STATIC_LEN(test_cases), test_cases);
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
    test_llvm_code_maps_ints(state, input, STATIC_LEN(cases), cases);
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
    test_llvm_code_maps_ints(state, input, STATIC_LEN(cases), cases);
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
    test_llvm_code_maps_ints(state, input, STATIC_LEN(cases), cases);
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
    test_llvm_code_maps_ints(state, input, STATIC_LEN(cases), cases);
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
    test_llvm_code_maps_ints(state, input, STATIC_LEN(cases), cases);
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
    test_llvm_code_maps_ints(state, input, STATIC_LEN(cases), cases);
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
    test_llvm_code_maps_ints(state, input, STATIC_LEN(cases), cases);
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
    test_llvm_code_maps_ints(state, input, STATIC_LEN(cases), cases);
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
    test_llvm_code_maps_ints(state, input, STATIC_LEN(cases), cases);
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
    test_llvm_code_maps_ints(state, input, STATIC_LEN(cases), cases);
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
    test_llvm_code_maps_ints(state, input, STATIC_LEN(cases), cases);
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
    test_llvm_code_maps_ints(state, input, STATIC_LEN(cases), cases);
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
    test_llvm_code_maps_ints(state, input, STATIC_LEN(cases), cases);
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
    test_llvm_code_maps_ints(state, input, STATIC_LEN(cases), cases);
  }
  test_end(state);
  test_group_end(state);

  if (!state->config.lite) {
    test_robustness(state);
  }

  test_group_end(state);
}
