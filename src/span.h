#pragma once

#include <stdbool.h>

#include "consts.h"

// sizeof: 8
typedef struct {
  buf_ind_t start;
  buf_ind_t len;
} span;

bool spans_equal(span a, span b);
