// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <hedley.h>

#if HEDLEY_GCC_VERSION_CHECK(11, 3, 0)
#define MALLOC_ATTR_1(deallocator)                                             \
  HEDLEY_MALLOC __attribute__((malloc(deallocator)))
#define MALLOC_ATTR_2(deallocator, ptr_index)                                  \
  HEDLEY_MALLOC __attribute__((malloc(deallocator, ptr_index)))
#else
#define MALLOC_ATTR_1(a) HEDLEY_MALLOC
#define MALLOC_ATTR_2(a, b) HEDLEY_MALLOC
#endif

#if HEDLEY_HAS_ATTRIBUTE(cold)
#define COLD_ATTR __attribute((cold))
#else
#define COLD_ATTR
#endif

// TODO verify that passing zero args is the same as not passing args to the
// attribute, and this actually works
#define NON_NULL_PARAMS HEDLEY_NON_NULL()
#define NON_NULL_ALL NON_NULL_PARAMS HEDLEY_RETURNS_NON_NULL
