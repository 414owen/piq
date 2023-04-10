// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include "binding.h"
#include "bitset.h"
#include "vec.h"
#include "hashmap.h"

typedef struct {
  bitset is_builtin;
  vec_str_ref bindings;
  vec_u32 shadows;
  a_hashmap map;
} scope;

typedef struct {
  scope *scope;
  const char *source_file;
} resolve_map_ctx;
