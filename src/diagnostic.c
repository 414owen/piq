#include <stdlib.h>
#include <string.h>
#include <hedley.h>

#include "consts.h"
#include "diagnostic.h"
#include "parse_tree.h"
#include "source.h"
#include "term.h"
#include "util.h"
#include "vec.h"

typedef struct {
  buf_ind_t start;
  buf_ind_t end;
} line;

NON_NULL_PARAMS
static vec_buf_ind get_line_starts(const char *restrict input, size_t len) {
  vec_buf_ind line_starts = VEC_NEW;
  VEC_PUSH(&line_starts, 0);
  for (buf_ind_t i = 0; i < len; i++) {
    if (input[i] == '\n') {
      VEC_PUSH(&line_starts, i + 1);
    }
  }
  return line_starts;
}

typedef enum {
  S_BEFORE,
  S_BLUE,
  S_AFTER,
} print_state;

position_info find_line_and_col(const char *restrict input, buf_ind_t target) {
  position_info res = {
    .line = 1,
    .column = 1,
  };
  for (buf_ind_t i = 0; i < target; i++) {
    char c = input[i];
    if (HEDLEY_PREDICT_FALSE(c == '\n', 0.9)) {
      res.line++;
      res.column = 1;
    } else {
      res.column++;
    }
  }
  return res;
}

void format_error_ctx(FILE *f, const char *restrict input, buf_ind_t start,
                      buf_ind_t len) {
  size_t buf_len = strlen(input);
  size_t reset_pos = start + len;
  vec_buf_ind line_starts = get_line_starts(input, buf_len);
  int64_t ind_of_line_starting_before_start = 0;

  for (unsigned i = 0; i < line_starts.len; i++) {
    buf_ind_t a = line_starts.data[i];
    if (a <= start) {
      ind_of_line_starting_before_start = i;
    }
  }

  fputs(RED "/---\n| " RESET, f);
  buf_ind_t context_start =
    line_starts
      .data[MAX(ind_of_line_starting_before_start - ERROR_LINES_CTX, 0)];

  print_state state = S_BEFORE;
  buf_ind_t nls_after = 0;
  for (buf_ind_t i = context_start; i < buf_len; i++) {
    char c = input[i];
    if (c == '\n') {
      if (state == S_AFTER) {
        if (nls_after == ERROR_LINES_CTX) {
          break;
        } else {
          nls_after++;
        }
      }
    }
    switch (state) {
      case S_BEFORE:
        if (i == start) {
          fputs(BLU, f);
          state = S_BLUE;
        }
        break;
      case S_BLUE:
        if (i == reset_pos) {
          fputs(RESET, f);
          state = S_AFTER;
        }
        break;
      case S_AFTER:
        break;
    }
    putc(c, f);
    if (c == '\n') {
      fputs(RED "| " RESET, f);
      if (state == S_BLUE) {
        fputs(BLU, f);
      }
    }
  }
  fputs("\n" RED "\\---" RESET, f);
  VEC_FREE(&line_starts);
}

void print_resolution_errors(FILE *f, const char *restrict input,
                             resolution_errors errs) {
  for (node_ind_t i = 0; i < errs.binding_amt; i++) {
    binding b = errs.bindings[i];
    position_info pos = find_line_and_col(input, b.start);
    fprintf(f,
            "%sUnknown binding '%.*s' at %u:%u",
            i == 0 ? "" : "\n",
            b.len,
            input + b.start,
            pos.line,
            pos.column);
  }
}

char *print_resolution_errors_string(const char *restrict input,
                                     resolution_errors errs) {
  stringstream ss;
  ss_init_immovable(&ss);
  print_resolution_errors(ss.stream, input, errs);
  ss_finalize(&ss);
  return ss.string;
}
