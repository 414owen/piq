// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include "traversal.h"

pt_traversal pt_walk(parse_tree tree, traverse_mode mode);
pt_traverse_elem pt_walk_next(pt_traversal *traversal);
