// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "defs.h"

#ifdef STD_C11
  typedef struct timespec timespec;
  typedef struct timespec difftime_t;

  #define TIMESPEC_ZERO {0}

  // timespec -> time_t
  #define difftime_to_secs(ts) ((long_long) (tv.tv_sec))

  // TODO only compile this function if running tests
  bool timespec_negative(timespec a);

  bool timespec_gt(timespec x, timespec y);

  difftime_t timespec_subtract(const timespec x,
                                    const timespec y);

  timespec timespec_add(const timespec x, const timespec y);

#else
  typedef clock_t timespec;
  typedef clock_t difftime_t;

  #define TIMESPEC_ZERO 0

  // timespec -> long long
  #define difftime_to_secs(ts) ((long long) ts)

  #define timespec_gt(a, b) (a > b)

  #define timespec_add(a, b) (a + b)

  #define timespec_subtract difftime
#endif

uint64_t timespec_to_nanoseconds(timespec);
