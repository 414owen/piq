// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <stdbool.h>

#include "consts.h"

// sizeof: 8
typedef struct {
  buf_ind_t start;
  buf_ind_t len;
} span;

bool spans_equal(span a, span b);
