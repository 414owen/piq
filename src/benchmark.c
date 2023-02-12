#include <stdlib.h>

#include "benchmark.h"
#include "util.h"

void run_benchmarks(test_state *state) {
  stringstream ss;
  ss_init_immovable(&ss);
  for (int i = 0; i < 1000; i++) {
    fputs(
      "(sig aFunction (Fn I32 I32 Bool))"
      "(fun aFunction ((a, e) (c, b, d))\n"
      "   (sig c0 I32)\n"
      "   (let c0 0)\n", ss.stream);
      const int statement_amt = 20;
      for (int i = 1; i < statement_amt - 1; i++) {
        const int backref = abs(rand()) % i;
        fprintf(
          ss.stream,
          "\n"
          "   (sig c%d I32)\n"
          "   (let c%d c%d)\n", i, i, backref);
      }
    fputs(")\n\n", ss.stream);
  }
  ss_finalize(&ss);

}
