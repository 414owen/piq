// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "attrs.h"
#include "diagnostic.h"
#include "typecheck.h"

COLD_ATTR
static void print_tc_error(FILE *f, tc_res res, const char *restrict input,
                           parse_tree tree, node_ind_t err_ind) {
  tc_error error = res.errors[err_ind];
  // TODO be informative, add provenance, etc.
  switch (error.type) {
    case TC_ERR_AMBIGUOUS:
      fputs("Ambiguous type: ", f);
      print_type(f,
                 res.types.tree.nodes,
                 res.types.tree.inds,
                 error.data.ambiguous.index);
      break;
    case TC_ERR_CONFLICT:
      fputs("Conflicting types:\nExpected:\n", f);
      print_type(f,
                 res.types.tree.nodes,
                 res.types.tree.inds,
                 error.data.conflict.expected_ind);
      fputs("\n\nGot:\n", f);
      print_type(f,
                 res.types.tree.nodes,
                 res.types.tree.inds,
                 error.data.conflict.got_ind);
      fputc('\n', f);
      break;
    case TC_ERR_INFINITE:
      fputs("Can't construct infinite type:\n", f);
      print_type(f,
                 res.types.tree.nodes,
                 res.types.tree.inds,
                 error.data.ambiguous.index);
      break;
  }
  parse_node node = tree.nodes[error.pos];
  position_info pos = find_line_and_col(input, node.phase_data.span.start);
  fprintf(f,
          "\nAt %s at %d:%d\n",
          parse_node_strings[node.type.all],
          pos.line,
          pos.column);
  format_error_ctx(
    f, input, node.phase_data.span.start, node.phase_data.span.len);
}

COLD_ATTR
void print_tc_errors(FILE *f, const char *restrict input, parse_tree tree,
                     tc_res res) {
  for (size_t i = 0; i < res.error_amt; i++) {
    putc('\n', f);
    print_tc_error(f, res, input, tree, i);
  }
}
