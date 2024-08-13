// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#pragma once

#include <stdint.h>
#include <time.h>
#include <predef/predef/os.h>

#include "util.h"
#include "timing.h"

#ifdef PREDEF_OS_LINUX
# include "platform/linux/perf.h"
#else

typedef struct {
  timespec start_time;
} perf_state;

#endif

typedef struct {
  timespec time_taken;

#ifdef PREDEF_OS_LINUX
  uint64_t hw_cpu_cycles;
  uint64_t hw_branch_mispredictions;
  uint64_t hw_cache_misses;
  uint64_t hw_retired_instructions;
  uint64_t hw_retired_branches;
#endif
} perf_values;

extern perf_values perf_zero;

perf_state perf_start(void);
perf_values perf_end(perf_state state);
perf_values perf_add(perf_values a, perf_values b);
