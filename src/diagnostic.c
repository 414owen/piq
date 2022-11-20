#include <stdlib.h>
#include <string.h>

#include "consts.h"
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

void format_error_ctx(FILE *f, const char *restrict input, buf_ind_t start,
                      buf_ind_t end) {
  size_t len = strlen(input);
  vec_buf_ind line_starts = get_line_starts(input, len);
  int64_t ind_of_line_starting_before_start = 0;

  for (unsigned i = 0; i < line_starts.len; i++) {
    buf_ind_t a = line_starts.data[i];
    if (a <= start) {
      ind_of_line_starting_before_start = i;
    }
  }

  fputs(RED "/---\n| " RESET, f);
  buf_ind_t context_start = line_starts.data[MAX(ind_of_line_starting_before_start - ERROR_LINES_CTX, 0)];
  
  print_state state = S_BEFORE;
  buf_ind_t nls_after = 0;
  for (buf_ind_t i = context_start; i < len; i++) {
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
      case S_AFTER:
        break;
    }
    putc(c, f);
    switch (state) {
      case S_BLUE:
        if (i == end) {
          fputs(RESET, f);
          state = S_AFTER;
        }
        break;
      case S_BEFORE:
      case S_AFTER:
        break;
    }

    if (c == '\n') {
      fputs(RED "| " RESET, f);
      if (state == S_BLUE) {
        fputs(BLU, f);
      }
    }
  }
  fputs("\n" RED "\\---" RESET, f);
}
