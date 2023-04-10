// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include "consts.h"
#include "vec.h"
#include "span.h"

typedef span binding;

VEC_DECL_CUSTOM(binding, vec_binding);

typedef union {
  const char *builtin;
  binding binding;
} str_ref;

VEC_DECL(str_ref);
