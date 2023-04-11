// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include "binding.h"
#include "bitset.h"
#include "parse_tree.h"
#include "hashmap.h"
#include "vec.h"

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

typedef struct {
  binding *bindings;
  node_ind_t binding_amt;
} resolution_errors;

typedef struct {
  resolution_errors not_found;
#ifdef TIME_NAME_RESOLUTION
  perf_values perf_values;
  u32 num_names_looked_up;
#endif
} resolution_res;

resolution_res resolve_bindings(parse_tree tree, const char *restrict input);
