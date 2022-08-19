#pragma once

#include <stdio.h>

#include "consts.h"

void format_error_ctx(FILE *f, const char *data, buf_ind_t start,
                      buf_ind_t end);
