// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stdbool.h>

#include "span.h"

bool spans_equal(span a, span b) {
  return a.start == b.start && a.len == b.len;
}
