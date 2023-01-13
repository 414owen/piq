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
      print_type(
        f, res.types.types, res.types.type_inds, error.ambiguous.index);
      break;
    case TC_ERR_CONFLICT:
      fputs("Conflicting types:\nExpected:\n", f);
      print_type(
        f, res.types.types, res.types.type_inds, error.conflict.expected_ind);
      fputs("\n\nGot:\n", f);
      print_type(
        f, res.types.types, res.types.type_inds, error.conflict.got_ind);
      fputc('\n', f);
      break;
    case TC_ERR_INFINITE:
      fputs("Can't construct infinite type:\n", f);
      print_type(
        f, res.types.types, res.types.type_inds, error.ambiguous.index);
      break;
  }
  parse_node node = tree.nodes[error.pos];
  position_info pos = find_line_and_col(input, node.span.start);
  fprintf(f, "\nAt: %d:%d\n", pos.line, pos.column);
  format_error_ctx(f, input, node.span.start, node.span.len);
}

COLD_ATTR
void print_tc_errors(FILE *f, const char *restrict input, parse_tree tree,
                     tc_res res) {
  for (size_t i = 0; i < res.error_amt; i++) {
    putc('\n', f);
    print_tc_error(f, res, input, tree, i);
  }
}
