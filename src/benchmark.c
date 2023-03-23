#include <stdlib.h>

#include "benchmark.h"
#include "test_upto.h"
#include "util.h"

#define INDENT_STR "   "
#define INDENT_STR2 INDENT_STR "  "
#define INDENT_STR3 INDENT_STR "    "
#define BINDING_STR "bndng"
#define FUNCTION_STR "test-fn-"

const int FUNCTION_AMT = 400;
const int STATEMENT_AMT = 400;

typedef uint32_t (*fn_type)(void);

static char *do_nothing(fn_type f, void *data) { return NULL; }

void run_benchmarks(test_state *state) {
  stringstream ss;
  ss_init_immovable(&ss);
  for (int i = 0; i < FUNCTION_AMT; i++) {
    fprintf(ss.stream,
            "(sig " FUNCTION_STR "%d (Fn I32 I32 I32))\n"
            "(fun " FUNCTION_STR "%d (a b)\n" INDENT_STR "(sig " BINDING_STR
            "0 I32)\n" INDENT_STR "(let " BINDING_STR "0 0)\n",
            i,
            i);
    for (int j = 1; j < STATEMENT_AMT - 1; j++) {
      const int backref = abs(rand()) % j;
      fprintf(ss.stream,
              "\n" INDENT_STR "(sig " BINDING_STR "%d I32)\n" INDENT_STR
              "(let " BINDING_STR "%d (i32-add " BINDING_STR "%d %d))\n",
              j,
              j,
              backref,
              j);
    }
    fputs(INDENT_STR "(let result\n", ss.stream);
    for (int j = 1; j < STATEMENT_AMT - 1; j++) {
      fprintf(ss.stream, INDENT_STR2 "(i32-add " BINDING_STR "%d\n", j);
    }
    fputs(INDENT_STR3 "1", ss.stream);
    for (int j = 1; j < STATEMENT_AMT - 1; j++) {
      fputc(')', ss.stream);
    }
    fputs(")\n", ss.stream);
    if (i == 0) {
      fputs(INDENT_STR "result", ss.stream);
    } else {
      fprintf(ss.stream, INDENT_STR "(" FUNCTION_STR "%d 2 result)", i - 1);
    }
    fputs(")\n\n", ss.stream);
  }
  fprintf(
    ss.stream, "(fun entry () (" FUNCTION_STR "%d 42 101))", FUNCTION_AMT - 1);

  ss_finalize(&ss);

  // FILE *f = fopen("bench.prog", "w");
  // fputs(ss.string, f);
  // fclose(f);
  // puts(ss.string);

  llvm_symbol_test test = {
    .symbol_name = "entry",
    .test_callback = do_nothing,
  };

  test_group_start(state, "Benchmark");
  test_start(state, "Benchmark");
  test_upto_codegen_with(state, ss.string, &test, 1);
  test_end(state);
  test_group_end(state);
}
