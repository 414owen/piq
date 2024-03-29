// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include "consts.h"

// Pretty similar to spans tbh

typedef struct {
  node_ind_t start;
  node_ind_t amt;
} node_slice;
