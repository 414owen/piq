#include "attrs.h"
#include "typecheck.h"

COLD_ATTR
static void print_tc_error(FILE *f, tc_res res, const char *restrict input,
                           parse_tree tree, node_ind_t err_ind) {
  tc_error error = res.errors[err_ind];
  // TODO be informative, add provenance, etc.
  switch (error.type) {
    case TC_ERR_AMBIGUOUS:
      fputs("Ambiguous type.", f);
      break;
    case TC_ERR_CONFLICT:
      fputs("Parsing conflict.", f);
      break;
    case TC_ERR_INFINITE:
      fputs("Can't construct infinite type", f);
      break;
  }
}

COLD_ATTR
void print_tc_errors(FILE *f, const char *restrict input, parse_tree tree,
                     tc_res res) {
  for (size_t i = 0; i < res.error_amt; i++) {
    putc('\n', f);
    print_tc_error(f, res, input, tree, i);
  }
}
