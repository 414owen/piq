// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include "util.h"

#if __STDC_VERSION__ >= 201112L
#include <assert.h> /* static_assert */
#else
#define static_assert(condition, msg) sizeof(char[1 - 2 * !!(condition)])
#endif

#define assert_array_maps_enum(enum_len, arr)                                  \
  static_assert(STATIC_LEN(arr) == (enum_len),                                 \
                "length(" #arr ") should equal " #enum_len                     \
                " because it maps an enum")
