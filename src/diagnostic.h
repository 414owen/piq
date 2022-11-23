#pragma once

#include <stdio.h>

#include "consts.h"

// start end length of highlighted segment of code
void format_error_ctx(FILE *f, const char *data, buf_ind_t start,
                      buf_ind_t len);
