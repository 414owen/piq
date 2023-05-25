// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "diagnostic.h"
#include "test.h"
#include "util.h"

const char *test_program = "(sig sndpar (Fn (I16, I32) I32))\n"
                           "(fun sndpar (a, b) b)\n"
                           "\n"
                           "(sig (Fn () I32))\n"
                           "(fun test () (sndpar (1, 2)))";

void test_diagnostics(test_state *state) {
  test_group_start(state, "Robustness");

  test_start(state, "previous context");
  {
    stringstream output;
    ss_init_immovable(&output);
    format_error_ctx(output.stream, test_program, 0, 1);
    ss_finalize(&output);
    size_t newlines = count_char_occurences(output.string, '\n');
    test_assert_eq(state, newlines, 4);
    free(output.string);
  }
  test_end(state);

  test_group_end(state);
}
