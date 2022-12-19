#pragma once

#include <stdio.h>

#include "consts.h"
#include "parse_tree.h"

typedef struct {
  buf_ind_t line;
  buf_ind_t column;
} position_info;

// start end length of highlighted segment of code
void format_error_ctx(FILE *f, const char *data, buf_ind_t start,
                      buf_ind_t len);
position_info find_line_and_col(const char *restrict input, buf_ind_t target);
void print_resolution_errors(FILE *f, const char *restrict input,
                             resolution_errors errs);
char *print_resolution_errors_string(const char *restrict input,
                                     resolution_errors errs);
